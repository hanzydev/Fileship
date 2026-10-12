import { filesize } from 'filesize';
import { z } from 'zod';

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const query = await readListQuery(event, {
        kind: z.enum(['files', 'folders', 'notes']).default('files'),
    });

    const { trashRetentionDays } = await getSettings();
    const { field, dir } = parseSort(query.sort, ['deletedAt', 'name'], 'deletedAt:desc');
    const contains = query.q ? { contains: query.q, mode: 'insensitive' as const } : undefined;
    const trashed = { authorId: currentUser.id, deletedAt: { not: null } };
    const page = { skip: query.offset, take: query.limit };

    const serialize = (item: { id: string; deletedAt: Date | null }, name: string) => ({
        id: item.id,
        name,
        deletedAt: item.deletedAt,
        purgeAt: getPurgeAt(item.deletedAt!, trashRetentionDays),
    });

    if (query.kind === 'files') {
        const where = { ...trashed, fileName: contains };
        const [files, total] = await Promise.all([
            prisma.file.findMany({
                where,
                orderBy: [field === 'name' ? { fileName: dir } : { deletedAt: dir }, { id: 'asc' }],
                select: { id: true, fileName: true, mimeType: true, size: true, deletedAt: true },
                ...page,
            }),
            prisma.file.count({ where }),
        ]);

        return {
            items: files.map((file) => ({
                ...serialize(file, file.fileName),
                mimeType: file.mimeType,
                size: { raw: file.size.toString(), formatted: filesize(file.size) },
            })),
            total,
        };
    }

    if (query.kind === 'folders') {
        const where = { ...trashed, name: contains };
        const [folders, total] = await Promise.all([
            prisma.folder.findMany({
                where,
                orderBy: [field === 'name' ? { name: dir } : { deletedAt: dir }, { id: 'asc' }],
                select: { id: true, name: true, deletedAt: true },
                ...page,
            }),
            prisma.folder.count({ where }),
        ]);

        return {
            items: await Promise.all(
                folders.map(async (folder) => ({
                    ...serialize(folder, folder.name),
                    fileCount: await prisma.file.count({ where: filesTrashedWith(folder) }),
                })),
            ),
            total,
        };
    }

    const where = { ...trashed, title: contains };
    const [notes, total] = await Promise.all([
        prisma.note.findMany({
            where,
            orderBy: [field === 'name' ? { title: dir } : { deletedAt: dir }, { id: 'asc' }],
            select: { id: true, title: true, deletedAt: true },
            ...page,
        }),
        prisma.note.count({ where }),
    ]);

    return {
        items: notes.map((note) => serialize(note, note.title)),
        total,
    };
});
