import { randomUUID } from 'node:crypto';

import { WebhookDeliveryStatus } from '#shared/prisma/enums';
import type { WebhookDeliveryModel, WebhookModel } from '#shared/prisma/models';

export const WEBHOOK_MAX_ATTEMPTS = 5;

const RETRY_DELAYS = [10_000, 60_000, 5 * 60_000, 30 * 60_000];
const REQUEST_TIMEOUT = 10_000;
const LEASE_MS = 2 * 60_000;
const ANALYSIS_WAIT_MS = 30 * 60_000;
const ANALYSIS_POLL_MS = 10_000;
const MAX_RETRY_AFTER = 5 * 60;

type Outcome = {
    status: WebhookDeliveryStatus;
    statusCode?: number | null;
    error?: string | null;
    detail?: string | null;
    duration?: number | null;
    retryIn?: number;
    rateLimited?: boolean;
    counts?: boolean;
};

const finished = [WebhookDeliveryStatus.Succeeded, WebhookDeliveryStatus.Failed];

const bodyRetryAfter = (body: string) => {
    try {
        return JSON.parse(body).retry_after as number | undefined;
    } catch {
        return undefined;
    }
};

export const claimWebhookDelivery = async (workerId: string) => {
    const now = new Date();
    const leaseCutoff = new Date(now.getTime() - LEASE_MS);

    const [delivery] = await prisma.$queryRaw<WebhookDeliveryModel[]>`
        UPDATE "WebhookDelivery"
        SET "status" = 'Processing',
            "lockedAt" = ${now.toISOString()}::timestamp,
            "lockedBy" = ${workerId},
            "attempts" = "attempts" + 1
        WHERE "id" = (
            SELECT d."id" FROM "WebhookDelivery" d
            JOIN "Webhook" w ON w."id" = d."webhookId"
            WHERE w."enabled"
              AND (w."retryAt" IS NULL OR w."retryAt" <= ${now.toISOString()}::timestamp)
              AND (
                  (d."status" = 'Pending' AND d."nextAttemptAt" <= ${now.toISOString()}::timestamp)
                  OR (d."status" = 'Processing' AND d."lockedAt" < ${leaseCutoff.toISOString()}::timestamp)
              )
              AND NOT EXISTS (
                  SELECT 1 FROM "WebhookDelivery" p
                  WHERE p."webhookId" = d."webhookId"
                    AND p."status" = 'Processing'
                    AND p."lockedAt" >= ${leaseCutoff.toISOString()}::timestamp
              )
            ORDER BY d."nextAttemptAt" ASC, d."createdAt" ASC
            LIMIT 1
            FOR UPDATE SKIP LOCKED
        )
        RETURNING *`;

    return delivery ?? null;
};

const pruneDeliveries = async (webhookId: string) => {
    const keep = await prisma.webhookDelivery.findMany({
        where: { webhookId, status: { in: finished } },
        orderBy: { finishedAt: 'desc' },
        take: WEBHOOK_HISTORY,
        select: { id: true },
    });

    await prisma.webhookDelivery.deleteMany({
        where: {
            webhookId,
            status: { in: finished },
            id: { notIn: keep.map((delivery) => delivery.id) },
        },
    });
};

export const cancelPendingDeliveries = (webhookId: string, error: string) =>
    prisma.webhookDelivery.updateMany({
        where: { webhookId, status: WebhookDeliveryStatus.Pending },
        data: { status: WebhookDeliveryStatus.Failed, error, finishedAt: new Date() },
    });

export const sendWebhookUpdate = async (webhookId: string) => {
    const webhook = await prisma.webhook.findUnique({
        where: { id: webhookId },
        include: lastDeliveryInclude,
    });

    if (webhook) sendToUser(webhook.userId, 'webhook:update', await serializeWebhook(webhook));
};

const recordFailure = async (webhook: WebhookModel) => {
    const { failureCount } = await prisma.webhook.update({
        where: { id: webhook.id },
        data: { failureCount: { increment: 1 } },
    });

    if (failureCount < WEBHOOK_FAILURE_LIMIT) return;

    const { count } = await prisma.webhook.updateMany({
        where: { id: webhook.id, enabled: true },
        data: { enabled: false, disabledAt: new Date() },
    });

    if (!count) return;

    await cancelPendingDeliveries(webhook.id, 'disabled');
    await pruneDeliveries(webhook.id);

    await createLog(null, {
        action: 'Disable Webhook',
        message: `Disabled webhook ${webhook.name} after ${failureCount} failed deliveries`,
        system: true,
    });
};

const classify = (
    response: Awaited<ReturnType<typeof sendWebhookRequest>>,
    duration: number,
): Outcome => {
    const base = { statusCode: response.status, duration };

    if (response.status >= 200 && response.status < 300)
        return { ...base, status: WebhookDeliveryStatus.Succeeded };

    const failed = {
        ...base,
        status: WebhookDeliveryStatus.Failed,
        error: 'status',
        detail: response.body || null,
    };

    if (response.status === 429) {
        const retryAfter = response.retryAfter ?? bodyRetryAfter(response.body) ?? 5;

        return {
            ...failed,
            retryIn: Math.min(Math.max(retryAfter, 1), MAX_RETRY_AFTER) * 1000,
            rateLimited: true,
        };
    }

    return response.status === 408 || response.status >= 500 ? { ...failed, retryIn: 0 } : failed;
};

const attempt = async (
    webhook: WebhookModel & {
        user: { id: string; username: string; language: string | null; theme: string };
    },
    delivery: Pick<WebhookDeliveryModel, 'id' | 'event'>,
    payload: WebhookPayload,
): Promise<Outcome> => {
    const { body, headers } = await renderWebhookRequest(webhook, delivery, payload);
    const started = Date.now();

    try {
        const response = await sendWebhookRequest(await decrypt(webhook.url), body, headers, {
            allowPrivate: (await getSettings()).webhookPrivateTargets,
            timeout: REQUEST_TIMEOUT,
        });

        return classify(response, Date.now() - started);
    } catch (error: any) {
        const reason: WebhookFailure =
            error instanceof WebhookRequestError ? error.reason : 'network';

        return {
            status: WebhookDeliveryStatus.Failed,
            error: reason,
            detail: error?.detail ?? null,
            duration: Date.now() - started,
            ...((reason === 'timeout' || reason === 'network') && { retryIn: 0 }),
        };
    }
};

const resolvePayload = async (
    webhook: WebhookModel & { user: { id: string; aiSettings: unknown } },
    delivery: WebhookDeliveryModel,
) => {
    const payload = delivery.payload as WebhookPayload;
    if (payload.event !== 'file.uploaded') return payload;

    const file = await prisma.file.findUnique({
        where: { id: payload.data.file.id, deletedAt: null },
        include: { folder: { select: { public: true } } },
    });

    if (!file) return 'removed';

    if (
        Date.now() - delivery.createdAt.getTime() < ANALYSIS_WAIT_MS &&
        (await isAwaitingAnalysis(file, webhook.user)) &&
        (await prisma.aIJob.count({
            where: { fileId: file.id, status: { in: ['Pending', 'Processing'] } },
        }))
    )
        return 'waiting';

    return {
        ...payload,
        data: {
            ...payload.data,
            file: await webhookFileData(file, webhook.user, payload.baseUrl!),
        },
    };
};

export const runWebhookDelivery = async (delivery: WebhookDeliveryModel, workerId: string) => {
    const owned = { id: delivery.id, lockedBy: workerId };

    const webhook = await prisma.webhook.findUnique({
        where: { id: delivery.webhookId },
        include: { user: true },
    });

    if (!webhook) return;

    const payload = await resolvePayload(webhook, delivery);

    if (payload === 'waiting') {
        await prisma.webhookDelivery.updateMany({
            where: owned,
            data: {
                status: WebhookDeliveryStatus.Pending,
                attempts: { decrement: 1 },
                lockedAt: null,
                lockedBy: null,
                nextAttemptAt: new Date(Date.now() + ANALYSIS_POLL_MS),
            },
        });
        return;
    }

    const outcome: Outcome =
        payload === 'removed'
            ? { status: WebhookDeliveryStatus.Failed, error: 'fileRemoved', counts: false }
            : await attempt(webhook, delivery, payload);

    const retries =
        outcome.retryIn !== undefined &&
        (outcome.rateLimited
            ? Date.now() - delivery.createdAt.getTime() < 24 * 60 * 60_000
            : delivery.attempts < WEBHOOK_MAX_ATTEMPTS);

    const result = {
        statusCode: outcome.statusCode ?? null,
        error: outcome.error ?? null,
        detail: outcome.detail ?? null,
        duration: outcome.duration ?? null,
        lockedAt: null,
        lockedBy: null,
    };

    if (retries) {
        const retryIn = outcome.rateLimited
            ? outcome.retryIn!
            : RETRY_DELAYS[delivery.attempts - 1]!;

        await prisma.webhookDelivery.updateMany({
            where: owned,
            data: {
                ...result,
                status: WebhookDeliveryStatus.Pending,
                nextAttemptAt: new Date(Date.now() + retryIn),
                ...(outcome.rateLimited && { attempts: { decrement: 1 } }),
            },
        });

        if (outcome.rateLimited)
            await prisma.webhook.update({
                where: { id: webhook.id },
                data: { retryAt: new Date(Date.now() + retryIn) },
            });

        await sendWebhookUpdate(webhook.id);
        return;
    }

    const { count } = await prisma.webhookDelivery.updateMany({
        where: owned,
        data: { ...result, status: outcome.status, finishedAt: new Date() },
    });

    if (!count) return;

    if (outcome.status === WebhookDeliveryStatus.Succeeded) {
        if (webhook.failureCount)
            await prisma.webhook.update({ where: { id: webhook.id }, data: { failureCount: 0 } });
    } else if (outcome.counts !== false) {
        await recordFailure(webhook);
    }

    await pruneDeliveries(webhook.id);
    await sendWebhookUpdate(webhook.id);
};

export const testWebhook = async (
    webhook: WebhookModel & {
        user: { id: string; username: string; language: string | null; theme: string };
    },
) => {
    const delivery = await prisma.webhookDelivery.create({
        data: {
            webhookId: webhook.id,
            event: 'webhook.test',
            status: WebhookDeliveryStatus.Processing,
            attempts: 1,
            lockedAt: new Date(),
            lockedBy: `test-${randomUUID()}`,
            payload: {
                id: randomUUID(),
                event: 'webhook.test',
                createdAt: new Date().toISOString(),
                data: { webhook: { id: webhook.id, name: webhook.name } },
            },
        },
    });

    const outcome = await attempt(webhook, delivery, delivery.payload as WebhookPayload);

    const updated = await prisma.webhookDelivery.update({
        where: { id: delivery.id },
        data: {
            status: outcome.status,
            statusCode: outcome.statusCode ?? null,
            error: outcome.error ?? null,
            detail: outcome.detail ?? null,
            duration: outcome.duration ?? null,
            lockedAt: null,
            lockedBy: null,
            finishedAt: new Date(),
        },
    });

    await pruneDeliveries(webhook.id);
    await sendWebhookUpdate(webhook.id);

    return serializeWebhookDelivery(updated);
};
