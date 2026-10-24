export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const limit = usableSpaceOf(currentUser);

    return {
        used: (await storageUsedBy(currentUser.id)).toString(),
        limit: limit === null ? null : limit.toString(),
    };
});
