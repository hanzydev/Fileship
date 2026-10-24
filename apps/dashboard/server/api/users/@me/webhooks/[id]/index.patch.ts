import { z } from 'zod';

import { WebhookFormat } from '#shared/prisma/enums';

const validationSchema = webhookInputSchema
    .extend({ url: webhookInputSchema.shape.url.or(z.literal('')) })
    .partial();

export default defineEventHandler(async (event) => {
    webhookUserOnly(event);

    const webhook = await findUserWebhook(event);
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) throw invalidBodyError(body.error);

    const { url, secret, enabled, ...fields } = body.data;
    const isJson = (body.data.format ?? webhook.format) === WebhookFormat.Json;

    if (isJson && (secret || !webhook.secret)) assertWebhookSecret(secret ?? '');
    if (url) await assertWebhookUrl(url);

    const isReenabled = enabled === true && !webhook.enabled;

    const updated = await prisma.webhook.update({
        where: { id: webhook.id },
        data: {
            ...fields,
            ...(url && { url: await encrypt(url) }),
            secret: isJson ? (secret ? await encrypt(secret) : webhook.secret) : null,
            ...(enabled !== undefined && { enabled }),
            ...(isReenabled && { failureCount: 0, disabledAt: null, retryAt: null }),
            language: requestLanguage(event),
        },
        include: lastDeliveryInclude,
    });

    if (enabled === false && webhook.enabled) await cancelPendingDeliveries(webhook.id, 'disabled');

    await createLog(event, {
        action: 'Update Webhook',
        message: `Updated webhook ${updated.name}`,
    });

    const serialized = await serializeWebhook(updated);

    sendToUser(webhook.userId, 'webhook:update', serialized);

    return serialized;
});
