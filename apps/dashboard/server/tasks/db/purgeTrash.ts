import consola from 'consola';

export default defineTask({
    meta: {
        name: 'db:purgeTrash',
        description: 'Deletes items that stayed in the trash longer than the retention period',
    },
    async run() {
        const { trashRetentionDays } = await getSettings();
        const expired = { deletedAt: { lte: getPurgeAt(new Date(), -trashRetentionDays) } };

        const { files, folders, notes } = await deleteTrashed({
            files: expired,
            folders: expired,
            notes: expired,
        });

        const total = files.length + folders.length + notes.length;
        if (!total) return { result: 0 };

        for (const authorId of new Set(
            [...files, ...folders, ...notes].map((item) => item.authorId),
        ))
            sendToUser(authorId, 'trash:update', null);

        await createLog(null, {
            action: 'Empty Trash',
            message: `Deleted ${files.length} files, ${folders.length} folders and ${notes.length} notes that stayed in the trash for ${trashRetentionDays} days`,
            system: true,
        });

        consola.info(`Purged ${total} items from the trash.`);

        return { result: total };
    },
});
