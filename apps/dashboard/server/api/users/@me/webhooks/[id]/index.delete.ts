export default defineEventHandler(async (event) => {
    webhookUserOnly(event);

    const webhook = await findUserWebhook(event);

    await prisma.webhook.delete({ where: { id: webhook.id } });

    await createLog(event, {
        action: 'Delete Webhook',
        message: `Deleted webhook ${webhook.name}`,
    });

    sendToUser(webhook.userId, 'webhook:delete', webhook.id);
});
