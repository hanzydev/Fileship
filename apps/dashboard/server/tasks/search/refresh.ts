let lastRefresh = new Date();

export default defineTask({
    meta: {
        name: 'search:refresh',
        description: 'Indexes files analyzed by another server since the last refresh',
    },
    async run() {
        if (!appState.ready) return { result: 'skipped' };

        const since = new Date(lastRefresh.getTime() - 2 * 60_000);
        lastRefresh = new Date();

        const files = await prisma.file.findMany({
            where: { aiAnalyzedAt: { gte: since } },
            select: { id: true },
        });

        for (const file of files) await upsertFileSearchEntry(file.id);

        return { result: 'success', refreshed: files.length };
    },
});
