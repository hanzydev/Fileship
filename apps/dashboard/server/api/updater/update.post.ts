export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    const isRestoring = await prisma.user.count({
        where: { backupRestoreState: { not: null } },
    });

    if (isRestoring) {
        throw createError({
            status: 409,
            message: 'A backup is being restored, try again when it finishes',
        });
    }

    await createLog(event, {
        action: 'Update Fileship',
        message: 'Started an update',
    });

    return sendProxy(event, `${updaterUrl}/api/update`, {
        fetchOptions: {
            method: 'POST',
            headers: {
                Authorization: updaterSecret,
            },
        },
    });
});
