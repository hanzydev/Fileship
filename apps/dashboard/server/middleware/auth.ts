export default defineEventHandler(async (event) => {
    event.context.sessionPrivateId =
        getHeader(event, 'Authorization') || getCookie(event, 'sessionId') || null;

    event.context.user = (await verifyUser(event.context.sessionPrivateId!)) as any;

    if (event.context.sessionPrivateId && !event.context.user) {
        deleteCookie(event, 'sessionId');
    }
});
