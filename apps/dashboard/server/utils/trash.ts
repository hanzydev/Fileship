import { z } from 'zod';

import { removeMultiple } from '@orama/orama';

import type { Prisma } from '~~/generated/prisma/client';

type StoredFile = { id: string; fileName: string; storageId: string | null };

export const trashSelectionSchema = z.object({
    files: z.array(z.uuid()).max(5000).default([]),
    folders: z.array(z.uuid()).max(5000).default([]),
    notes: z.array(z.uuid()).max(5000).default([]),
});

export const getPurgeAt = (deletedAt: Date, retentionDays: number) =>
    new Date(deletedAt.getTime() + retentionDays * 24 * 60 * 60 * 1000);

export const isTrashEnabled = async () => (await getSettings()).trashRetentionDays > 0;

export const deleteFilesForever = async (files: StoredFile[]) => {
    await prisma.file.deleteMany({ where: { id: { in: files.map((file) => file.id) } } });
    await removeFileData(files);
};

export const filesTrashedWith = (folder: { id: string; deletedAt: Date | null }) => ({
    folderId: folder.id,
    deletedAt: folder.deletedAt ?? undefined,
    NOT: { deletedAt: null },
});

export const deleteFoldersForever = async (folderIds: string[]) => {
    const folders = await prisma.folder.findMany({
        where: { id: { in: folderIds }, deletedAt: { not: null } },
        select: { id: true, deletedAt: true },
    });

    if (folders.length)
        await deleteFilesForever(
            await prisma.file.findMany({
                where: { OR: folders.map(filesTrashedWith) },
                select: { id: true, fileName: true, storageId: true },
            }),
        );

    await prisma.folder.deleteMany({ where: { id: { in: folderIds } } });
    await removeMultiple(folderSearchDb, folderIds);
};

export const deleteNotesForever = async (noteIds: string[]) => {
    await prisma.note.deleteMany({ where: { id: { in: noteIds } } });
    await removeMultiple(noteSearchDb, noteIds);
};

export const deleteTrashed = async (where: {
    files: Prisma.FileWhereInput;
    folders: Prisma.FolderWhereInput;
    notes: Prisma.NoteWhereInput;
}) => {
    const [files, folders, notes] = await Promise.all([
        prisma.file.findMany({
            where: where.files,
            select: { id: true, fileName: true, storageId: true, authorId: true },
        }),
        prisma.folder.findMany({ where: where.folders, select: { id: true, authorId: true } }),
        prisma.note.findMany({ where: where.notes, select: { id: true, authorId: true } }),
    ]);

    await deleteFilesForever(files);
    await deleteFoldersForever(folders.map((folder) => folder.id));
    await deleteNotesForever(notes.map((note) => note.id));

    return { files, folders, notes };
};
