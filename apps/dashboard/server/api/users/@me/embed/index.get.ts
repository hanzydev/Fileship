export default defineEventHandler(async (event) => {
    userOnly(event);
    return embedOf(event.context.user!.embed);
});
