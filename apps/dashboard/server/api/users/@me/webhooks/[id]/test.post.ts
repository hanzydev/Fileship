export default defineEventHandler(async (event) => {
    webhookUserOnly(event);

    const webhook = await findUserWebhook(event);

    await rateLimit(`webhookTest:${webhook.userId}`, 5, 60_000);

    return testWebhook({ ...webhook, user: event.context.user! });
});
