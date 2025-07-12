export default defineEventHandler(async (event) => {
    userOnly(event);
    return event.context.user!.domains;
});
