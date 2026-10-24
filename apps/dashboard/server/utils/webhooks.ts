import { createHmac, randomUUID } from 'node:crypto';

import consola from 'consola';
import type { H3Event } from 'h3';
import { z } from 'zod';

import { WebhookDeliveryStatus, WebhookFormat } from '#shared/prisma/enums';
import type {
    FileModel,
    LinkModel,
    NoteModel,
    WebhookDeliveryModel,
    WebhookModel,
} from '#shared/prisma/models';
import type { Prisma } from '~~/generated/prisma/client';

import pkg from '../../../../package.json';

export type WebhookVisibility = 'public' | 'locked' | 'hidden';
export type WebhookDeleteReason = 'trashed' | 'deleted' | 'expired' | 'maxViews';

export type WebhookPayload = {
    id: string;
    event: WebhookEvent | 'webhook.test';
    createdAt: string;
    baseUrl?: string;
    data: Record<string, any>;
};

export type WebhookFile = Pick<
    FileModel,
    | 'id'
    | 'fileName'
    | 'name'
    | 'mimeType'
    | 'size'
    | 'password'
    | 'piiDetected'
    | 'expiresAt'
    | 'aiAnalyzedAt'
> & { folder?: { public: boolean } | null };

type WebhookOwner = { id: string; aiSettings: unknown };

const webhookEventSchema = z.enum(Object.keys(WEBHOOK_EVENTS) as [WebhookEvent]);

export const webhookInputSchema = z.object({
    name: z.string().trim().min(1, 'errorsWebhooks.nameRequired').max(64, 'errorsWebhooks.nameMax'),
    url: z
        .url({ protocol: /^https?$/, message: 'errorsWebhooks.urlInvalid' })
        .max(2048, 'errorsWebhooks.urlMax'),
    format: z.enum(WebhookFormat),
    events: z.array(webhookEventSchema).min(1, 'errorsWebhooks.eventsRequired'),
    secret: z.string().trim().max(256, 'errorsWebhooks.secretMax'),
    enabled: z.boolean(),
});

export const isAwaitingAnalysis = async (
    file: Pick<FileModel, 'fileName' | 'aiAnalyzedAt'>,
    owner: WebhookOwner,
) =>
    !file.aiAnalyzedAt &&
    ['image', 'text'].includes(getAIFileKind(file.fileName) ?? '') &&
    (owner.aiSettings as any)?.enabled !== false &&
    (await isAiEnabled());

const fileVisibility = async (
    file: WebhookFile,
    owner: WebhookOwner,
): Promise<WebhookVisibility> => {
    if (
        isHiddenFromVisitors(file) ||
        file.folder?.public === false ||
        (await isAwaitingAnalysis(file, owner))
    )
        return 'hidden';

    return file.password ? 'locked' : 'public';
};

const deletedFileData = async (file: WebhookFile, owner: WebhookOwner) => {
    const visibility = await fileVisibility(file, owner);

    return {
        id: file.id,
        fileName: file.fileName,
        name: visibility === 'public' ? file.name : file.fileName,
        mimeType: file.mimeType,
        size: Number(file.size),
        visibility,
    };
};

export const webhookFileData = async (
    file: WebhookFile & Pick<FileModel, 'createdAt'>,
    owner: WebhookOwner,
    baseUrl: string,
) => {
    const data = await deletedFileData(file, owner);

    return {
        ...data,
        url: `${baseUrl}/u/${file.fileName}`,
        viewUrl: `${baseUrl}/view/${file.fileName}`,
        thumbnailUrl:
            data.visibility === 'public' && canHaveThumbnail(file.mimeType) && hasThumbnail(file.id)
                ? `${baseUrl}/u/${file.fileName}/thumbnail`
                : null,
        createdAt: file.createdAt.toISOString(),
    };
};

const publicBaseUrl = (event: H3Event, domains: string[]) =>
    buildPublicUrl(event, domains, '/').slice(0, -1);

const queueWebhookEvent = async (
    userId: string,
    name: WebhookEvent,
    build: () => Promise<Omit<WebhookPayload, 'id' | 'event' | 'createdAt'> | null>,
) => {
    const webhooks = await prisma.webhook.findMany({
        where: { userId, enabled: true, events: { has: name } },
        select: { id: true },
    });

    if (!webhooks.length) return;

    const built = await build();
    if (!built) return;

    const payload: WebhookPayload = {
        id: randomUUID(),
        event: name,
        createdAt: new Date().toISOString(),
        ...built,
    };

    await prisma.webhookDelivery.createMany({
        data: webhooks.map((webhook) => ({ webhookId: webhook.id, event: name, payload })),
    });
};

export const emitWebhookEvent = (
    event: H3Event | null,
    userId: string,
    name: WebhookEvent,
    build: () => Promise<Omit<WebhookPayload, 'id' | 'event' | 'createdAt'> | null>,
) => {
    const task = queueWebhookEvent(userId, name, build).catch((error) =>
        consola.error(`Queueing webhook event ${name} failed: ${error?.message ?? error}`),
    );

    event?.waitUntil(task);
};

export const emitFileUploaded = (
    event: H3Event,
    owner: Parameters<typeof usableSpaceOf>[0] & { domains: string[] },
    file: { id: string; size: bigint },
    source: 'upload' | 'inbox',
) => {
    emitWebhookEvent(event, owner.id, 'file.uploaded', async () => ({
        baseUrl: publicBaseUrl(event, owner.domains),
        data: { source, file: { id: file.id } },
    }));

    emitWebhookEvent(event, owner.id, 'storage.almostFull', async () => {
        const limit = usableSpaceOf(owner);
        if (!limit) return null;

        const used = await storageUsedBy(owner.id);
        const threshold = (limit * BigInt(STORAGE_ALERT_PERCENT)) / 100n;

        if (used < threshold || used - file.size >= threshold) return null;

        return {
            data: {
                storage: {
                    used: Number(used),
                    limit: Number(limit),
                    percent: Math.floor(Number((used * 1000n) / limit)) / 10,
                    url: buildPublicUrl(event, [], '/dashboard/settings/storage'),
                },
            },
        };
    });
};

export const emitFilesDeleted = (
    event: H3Event | null,
    ownerId: string,
    files: WebhookFile[],
    reason: WebhookDeleteReason,
) => {
    if (!files.length) return;

    emitWebhookEvent(event, ownerId, 'file.deleted', async () => {
        const owner = await prisma.user.findUniqueOrThrow({
            where: { id: ownerId },
            select: { id: true, aiSettings: true },
        });

        return {
            data: {
                reason,
                count: files.length,
                files: await Promise.all(
                    files.slice(0, 100).map((file) => deletedFileData(file, owner)),
                ),
            },
        };
    });
};

export const emitLinkCreated = (
    event: H3Event,
    userId: string,
    link: LinkModel & { shortUrl: string },
) =>
    emitWebhookEvent(event, userId, 'link.created', async () => ({
        data: {
            link: {
                id: link.id,
                code: link.code,
                url: link.shortUrl,
                target: link.password ? null : link.url,
                visibility: link.password ? 'locked' : 'public',
                maxClicks: link.maxClicks,
                expiresAt: link.expiresAt?.toISOString() ?? null,
                createdAt: link.createdAt.toISOString(),
            },
        },
    }));

export const emitNoteCreated = (
    event: H3Event,
    userId: string,
    note: NoteModel & { publicUrl?: string },
) =>
    emitWebhookEvent(event, userId, 'note.created', async () => ({
        data: {
            note: {
                id: note.id,
                title: note.public ? note.title : null,
                url: note.publicUrl ?? null,
                visibility: note.public ? 'public' : 'hidden',
                createdAt: note.createdAt.toISOString(),
            },
        },
    }));

export const signWebhookBody = (secret: string, timestamp: number, body: string) =>
    `sha256=${createHmac('sha256', secret).update(`${timestamp}.${body}`).digest('hex')}`;

export const renderWebhookRequest = async (
    webhook: WebhookModel & {
        user: { id: string; username: string; language: string | null; theme: string };
    },
    delivery: Pick<WebhookDeliveryModel, 'id' | 'event'>,
    payload: WebhookPayload,
) => {
    const { baseUrl: _, ...event } = payload;
    const timestamp = Math.floor(Date.now() / 1000);

    const body = JSON.stringify(
        webhook.format === WebhookFormat.Json
            ? {
                  version: 1,
                  id: event.id,
                  event: event.event,
                  createdAt: event.createdAt,
                  webhook: { id: webhook.id, name: webhook.name },
                  user: { id: webhook.user.id, username: webhook.user.username },
                  data: event.data,
              }
            : formatWebhookMessage(webhook.format, payload, {
                  language: webhook.user.language ?? webhook.language,
                  theme: webhook.user.theme,
                  siteName: (await getSettings()).siteName,
              }),
    );

    return {
        body,
        headers: {
            'Content-Type': 'application/json',
            'User-Agent': `Fileship-Webhooks/${pkg.version}`,
            'X-Fileship-Event': delivery.event,
            'X-Fileship-Delivery': delivery.id,
            ...(webhook.format === WebhookFormat.Json &&
                webhook.secret && {
                    'X-Fileship-Timestamp': String(timestamp),
                    'X-Fileship-Signature': signWebhookBody(
                        await decrypt(webhook.secret),
                        timestamp,
                        body,
                    ),
                }),
        },
    };
};

export const serializeWebhookDelivery = (delivery: WebhookDeliveryModel) => ({
    id: delivery.id,
    event: delivery.event,
    status: delivery.status,
    attempts: delivery.attempts,
    statusCode: delivery.statusCode,
    error: delivery.error,
    detail: delivery.detail,
    duration: delivery.duration,
    nextAttemptAt: delivery.nextAttemptAt,
    createdAt: delivery.createdAt,
    finishedAt: delivery.finishedAt,
});

export const serializeWebhook = async (
    webhook: WebhookModel & { deliveries?: WebhookDeliveryModel[] },
) => ({
    id: webhook.id,
    name: webhook.name,
    host: new URL(await decrypt(webhook.url)).host,
    format: webhook.format,
    events: webhook.events,
    enabled: webhook.enabled,
    hasSecret: !!webhook.secret,
    failureCount: webhook.failureCount,
    disabledAt: webhook.disabledAt,
    createdAt: webhook.createdAt,
    lastDelivery: webhook.deliveries?.[0] ? serializeWebhookDelivery(webhook.deliveries[0]) : null,
});

export const lastDeliveryInclude = {
    deliveries: {
        where: {
            status: { in: [WebhookDeliveryStatus.Succeeded, WebhookDeliveryStatus.Failed] },
        },
        orderBy: { finishedAt: 'desc' },
        take: 1,
    },
} satisfies Prisma.WebhookInclude;

export const findUserWebhook = async (event: H3Event) => {
    const webhook = await prisma.webhook.findUnique({
        where: { id: getRouterParam(event, 'id'), userId: event.context.user!.id },
    });

    if (!webhook) throw apiError(404, 'errorsWebhooks.notFound');

    return webhook;
};

const webhookFieldError = (field: string, key: string) =>
    apiError(400, key, undefined, { formErrors: { [field]: { _errors: [translate(key)] } } });

export const assertWebhookSecret = (secret: string) => {
    if (secret.length < 16) throw webhookFieldError('secret', 'errorsWebhooks.secretRequired');
};

export const assertWebhookUrl = async (url: string) => {
    const { webhookPrivateTargets } = await getSettings();

    await assertWebhookTarget(url, webhookPrivateTargets).catch(() => {
        throw webhookFieldError('url', 'errorsWebhooks.privateAddress');
    });
};

export const webhookUserOnly = (event: H3Event) => {
    userOnly(event);

    if (!canUploadFiles(event) && !canTakeNotes(event) && !canShortenLinks(event))
        throw forbiddenError();
};

export const requestLanguage = (event: H3Event) =>
    ((event.context.i18n as any)?.locale as string | undefined) ?? 'en';
