export default defineEventHandler(async (event) => {
    adminOnly(event);

    const isRestoring = await prisma.user.count({
        where: { backupRestoreState: { not: null } },
    });

    if (isRestoring) {
        throw createError({
            statusCode: 409,
            message: 'A backup is being restored, try again when it finishes',
        });
    }

    return sendProxy(event, `${updaterUrl}/api/update`, {
        fetchOptions: {
            method: 'POST',
            headers: {
                Authorization: updaterSecret,
            },
        },
    });
});
