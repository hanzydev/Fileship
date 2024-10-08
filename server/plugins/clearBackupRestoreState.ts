export default defineNitroPlugin(() => {
    prisma.user.updateMany({
        data: {
            backupRestoreState: null,
        },
    });
});
