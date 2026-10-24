export default defineEventHandler(async (event) => {
    webhookUserOnly(event);

    const webhooks = await prisma.webhook.findMany({
        where: { userId: event.context.user!.id },
        orderBy: { createdAt: 'asc' },
        include: lastDeliveryInclude,
    });

    return Promise.all(webhooks.map(serializeWebhook));
});
