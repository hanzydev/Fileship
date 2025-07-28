import pkg from '../../../../package.json';

export default defineNitroPlugin(async () => {
    await telemetry.collectSystemInfo();

    const collectFileshipInfo = async () => {
        const [files, codes, folders, notes, users] = await prisma.$transaction([
            prisma.file.count(),
            prisma.code.count(),
            prisma.folder.count(),
            prisma.note.count(),
            prisma.user.count(),
        ]);

        await telemetry.collectFileshipInfo({
            version: pkg.version,
            files,
            codes,
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
