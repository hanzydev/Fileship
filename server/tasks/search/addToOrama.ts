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
            },
        });

        const codes = await prisma.code.findMany({
            select: {
                id: true,
                title: true,
                language: true,
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

        const urls = await prisma.url.findMany({
            select: {
                id: true,
                vanity: true,
                destinationUrl: true,
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

        await insertMultiple(fileSearchDb, files as never[]);
        await insertMultiple(codeSearchDb, codes);
        await insertMultiple(folderSearchDb, folders);
        await insertMultiple(noteSearchDb, notes);
        await insertMultiple(urlSearchDb, urls);
        await insertMultiple(userSearchDb, users);
        await insertMultiple(logSearchDb, logs);

        consola.success(
            `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Successfully added data to Orama search database.`,
        );

        return { result: 'success' };
    },
});
