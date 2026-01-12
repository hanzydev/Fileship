import consola from 'consola';
import dayjs from 'dayjs';

import { insertMultiple } from '@orama/orama';

export default defineTask({
    meta: {
        name: 'search:addToOrama',
        description: 'Adds data to Orama search database',
    },
    async run() {
        consola.info(
            `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Adding data to Orama search database...`,
        );

        const files = await prisma.file.findMany({
            select: {
                id: true,
                fileName: true,
                mimeType: true,
                embedding: true,
                textEmbedding: true,
                caption: true,
                ocrText: true,
            },
        });

        const folders = await prisma.folder.findMany({
            select: {
                id: true,
                name: true,
            },
        });

        const notes = await prisma.note.findMany({
            select: {
                id: true,
                title: true,
            },
        });

        const users = await prisma.user.findMany({
            select: {
                id: true,
                username: true,
            },
        });

        const logs = await prisma.log.findMany({
            select: {
                id: true,
                action: true,
                message: true,
                ip: true,
            },
        });

        await insertMultiple(
            fileSearchDb,
            files.map((file) => ({
                ...file,
                embedding: (file.embedding as number[]) || undefined,
                textEmbedding: (file.textEmbedding as number[]) || undefined,
                caption: file.caption || undefined,
                ocrText: file.ocrText || undefined,
            })),
        );
        await insertMultiple(folderSearchDb, folders);
        await insertMultiple(noteSearchDb, notes);
        await insertMultiple(userSearchDb, users);
        await insertMultiple(logSearchDb, logs);

        consola.success(
            `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Successfully added data to Orama search database.`,
        );

        return { result: 'success' };
    },
});
