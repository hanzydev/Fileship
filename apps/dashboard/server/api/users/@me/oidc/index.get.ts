export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const identity = await findOidcIdentity(currentUser.id, await getSettings());

    return {
        hasPassword: !!currentUser.password,
        identity: identity && { email: identity.email, createdAt: identity.createdAt },
    };
});
