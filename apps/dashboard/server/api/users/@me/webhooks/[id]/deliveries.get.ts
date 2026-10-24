import { WebhookDeliveryStatus } from '#shared/prisma/enums';

export default defineEventHandler(async (event) => {
    webhookUserOnly(event);

    const webhook = await findUserWebhook(event);

    const [items, pending] = await prisma.$transaction([
        prisma.webhookDelivery.findMany({
            where: { webhookId: webhook.id },
            orderBy: { createdAt: 'desc' },
            take: WEBHOOK_HISTORY,
        }),
        prisma.webhookDelivery.count({
            where: {
                webhookId: webhook.id,
                status: { in: [WebhookDeliveryStatus.Pending, WebhookDeliveryStatus.Processing] },
            },
        }),
    ]);

    return { pending, items: items.map(serializeWebhookDelivery) };
});
