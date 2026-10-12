import { promises as fsp } from 'node:fs';

import consola from 'consola';
import { join } from 'pathe';

export default defineTask({
    meta: {
        name: 'storage:sync',
        description: 'Removes files whose stored data is missing',
    },
    async run() {
        const stored = new Set(await fsp.readdir(join(dataDirectory, 'uploads')));
        const missing: { id: string; fileName: string; authorId: string }[] = [];

        let cursor: string | undefined;

        while (true) {
            const files = await prisma.file.findMany({
                select: { id: true, fileName: true, authorId: true },
                orderBy: { id: 'asc' },
                take: 500,
                ...(cursor && { skip: 1, cursor: { id: cursor } }),
            });

            if (!files.length) break;

            missing.push(...files.filter((file) => !stored.has(file.fileName)));
            cursor = files.at(-1)!.id;
        }

        if (!missing.length) return { result: 0 };

        if (!stored.size) {
            consola.warn(
                'The uploads storage is empty but the database has files. Skipping cleanup, check the data directory.',
            );

            return { result: 0 };
        }

        await prisma.file.deleteMany({ where: { id: { in: missing.map((file) => file.id) } } });
        await removeFileData(missing);

        for (const file of missing) sendToUser(file.authorId, 'file:delete', file.id);

        const names = missing.map((file) => file.fileName).join(', ');

        await createLog(null, {
            action: 'Delete File',
            message: `Removed ${missing.length} files missing from storage: ${names}`,
            system: true,
        });

        consola.info(`Removed ${missing.length} files missing from storage.`);

        return { result: missing.length };
    },
});
