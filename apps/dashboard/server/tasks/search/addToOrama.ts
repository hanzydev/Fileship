import consola from 'consola';
import dayjs from 'dayjs';

import { type AnyOrama, getByID, insertMultiple } from '@orama/orama';

export default defineTask({
    meta: {
        name: 'search:addToOrama',
        description: 'Adds data to Orama search database',
    },
    async run() {
        consola.info(
            `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Adding data to Orama search database...`,
        );

        const files = await prisma.file.findMany({ select: fileSearchSelect });

        const folders = await prisma.folder.findMany({
            select: {
                id: true,
                authorId: true,
                name: true,
            },
        });

        const notes = await prisma.note.findMany({
            select: {
                id: true,
                authorId: true,
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

        const missing = <T extends { id: string }>(db: AnyOrama, docs: T[]) =>
            docs.filter((doc) => !getByID(db, doc.id));

        await insertMultiple(fileSearchDb, missing(fileSearchDb, files).map(toFileSearchDoc));
        await insertMultiple(folderSearchDb, missing(folderSearchDb, folders));
        await insertMultiple(noteSearchDb, missing(noteSearchDb, notes));
        await insertMultiple(userSearchDb, missing(userSearchDb, users));
        await insertMultiple(logSearchDb, missing(logSearchDb, logs));

        consola.success(
            `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Successfully added data to Orama search database.`,
        );

        return { result: 'success' };
    },
});
