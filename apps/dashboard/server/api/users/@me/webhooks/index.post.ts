import { WebhookFormat } from '#shared/prisma/enums';

export default defineEventHandler(async (event) => {
    webhookUserOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, webhookInputSchema.safeParse);

    if (!body.success) throw invalidBodyError(body.error);

    if ((await prisma.webhook.count({ where: { userId: currentUser.id } })) >= WEBHOOK_LIMIT) {
        throw apiError(400, 'errorsWebhooks.limitReached', { limit: WEBHOOK_LIMIT });
    }

    const isJson = body.data.format === WebhookFormat.Json;

    if (isJson) assertWebhookSecret(body.data.secret);

    await assertWebhookUrl(body.data.url);

    const webhook = await prisma.webhook.create({
        data: {
            name: body.data.name,
            url: await encrypt(body.data.url),
            secret: isJson ? await encrypt(body.data.secret) : null,
            format: body.data.format,
            events: body.data.events,
            enabled: body.data.enabled,
            language: requestLanguage(event),
            userId: currentUser.id,
        },
        include: lastDeliveryInclude,
    });

    await createLog(event, {
        action: 'Create Webhook',
        message: `Created webhook ${webhook.name}`,
    });

    const serialized = await serializeWebhook(webhook);

    sendToUser(currentUser.id, 'webhook:create', serialized);

    return serialized;
});
