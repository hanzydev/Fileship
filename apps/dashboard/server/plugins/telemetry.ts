import pkg from '../../../../package.json';

export default defineNitroPlugin(async () => {
    await telemetry.collectSystemInfo();

    const collectFileshipInfo = async () => {
        const [files, folders, notes, users] = await prisma.$transaction([
            prisma.file.count(),
            prisma.folder.count(),
            prisma.note.count(),
            prisma.user.count(),
        ]);

        await telemetry.collectFileshipInfo({
            version: pkg.version,
            files,
            folders,
            notes,
            users,
        });
    };

    await collectFileshipInfo();

    setInterval(
        collectFileshipInfo,
        1000 * 60 * 30, // every 30 minutes
    );
});
