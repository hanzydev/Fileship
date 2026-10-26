import { createWriteStream, promises as fsp } from 'node:fs';
import { pipeline } from 'node:stream/promises';

import consola from 'consola';
import type { H3Event } from 'h3';
import { nanoid } from 'nanoid';
import { basename, dirname, join } from 'pathe';
import { z } from 'zod';

import { insert } from '@orama/orama';

import { UserPermission } from '#shared/prisma/enums';
import type { User } from '~~/generated/prisma/client';

import type { StoredObject } from '../storage/types';
import type { ImportSource } from './appImportSources';
import type { StorageImportProgress } from './storageImport';

const IMPORT_PERMISSIONS = [
    UserPermission.UploadFiles,
    UserPermission.TakeNotes,
    UserPermission.ShortenLinks,
];

export const importSourcePath = (id: string) => join(dataDirectory, 'temp', 'app-import', id);

export const saveImportSource = async (event: H3Event) => {
    const id = nanoid();
    const path = importSourcePath(id);

    await fsp.mkdir(dirname(path), { recursive: true });
    await pipeline(event.node.req, createWriteStream(path));

    try {
        await readImportSource(path);
    } catch (error) {
        await fsp.rm(path, { force: true });
        throw error;
    }

    return id;
};

export const loadImportSource = async (id: string) => {
    const path = importSourcePath(id);

    if (!(await fsp.stat(path).catch(() => null))) {
        throw apiError(404, 'errorsSystem.importSourceMissing');
    }

    return readImportSource(path);
};

const createObjectFinder = async (storageId: string | null) => {
    const byName = new Map<string, (readonly [string, StoredObject])[]>();
    const claimed = new Set<string>();

    for (const entry of await listImportObjects(storageId)) {
        const name = basename(entry[0]);
        byName.set(name, [...(byName.get(name) ?? []), entry]);
    }

    return (file: ImportSource['files'][number], claim = false) => {
        const path = file.path.replace(/^\/+/, '');
        const [found] = (byName.get(basename(path)) ?? [])
            .filter(
                ([key, object]) =>
                    !claimed.has(key) &&
                    (key === path || key.endsWith(`/${path}`)) &&
                    (file.size === null || object.size === file.size),
            )
            .sort(([a], [b]) => a.length - b.length);

        if (found && claim) claimed.add(found[0]);

        return found;
    };
};

const findMatchingUsers = async (usernames: string[]) => {
    const users = await prisma.user.findMany({
        where: { username: { in: usernames, mode: 'insensitive' } },
        select: { id: true, username: true },
    });

    return (username: string) =>
        users.find((user) => user.username === username) ??
        users.find((user) => user.username.toLowerCase() === username.toLowerCase()) ??
        null;
};

export const previewAppImport = async (sourceId: string, storageId: string | null) => {
    const source = await loadImportSource(sourceId);
    const findObject = await createObjectFinder(storageId);
    const matchUser = await findMatchingUsers(source.users.map((user) => user.username));
    const found = source.files.map((file) => findObject(file)).filter((entry) => !!entry);
    const countOf = (items: { userId: string | null }[], userId: string) =>
        items.filter((item) => item.userId === userId).length;

    return {
        app: source.app,
        version: source.version,
        files: source.files.length,
        found: found.length,
        size: found.reduce((total, [, object]) => total + object.size, 0),
        folders: source.folders.length,
        links: source.links.length,
        notes: source.notes.length,
        users: source.users.map((user) => ({
            id: user.id,
            username: user.username,
            files: countOf(source.files, user.id),
            links: countOf(source.links, user.id),
            notes: countOf(source.notes, user.id),
            keepsPassword: !!user.passwordHash,
            match: matchUser(user.username),
        })),
    };
};

const availableUsername = async (username: string) => {
    const base = username.trim().slice(0, 24).padEnd(3, '_');

    for (let index = 1; ; index++) {
        const suffix = index > 1 ? `-${index}` : '';
        const candidate = `${base.slice(0, 24 - suffix.length)}${suffix}`;
        const isTaken = await prisma.user.count({
            where: { username: { equals: candidate, mode: 'insensitive' } },
        });

        if (!isTaken) return candidate;
    }
};

export const resolveImportOwners = async (
    source: ImportSource,
    mapping: Record<string, string | null>,
) => {
    const owners = new Map<string, User>();

    for (const user of source.users) {
        const target = mapping[user.id];
        if (!target) continue;

        if (target !== 'new') {
            const existing = await prisma.user.findUnique({ where: { id: target } });
            if (existing) owners.set(user.id, existing);
            continue;
        }

        const email = z.email().safeParse(user.email?.trim().toLowerCase());
        const created = await createUser({
            username: await availableUsername(user.username),
            password: nanoid(32),
            passwordHash: user.passwordHash,
            permissions: IMPORT_PERMISSIONS,
            limits: { usableSpace: user.usableSpace },
            email:
                email.success && !(await prisma.user.count({ where: { email: email.data } }))
                    ? email.data
                    : undefined,
        });

        owners.set(user.id, await prisma.user.findUniqueOrThrow({ where: { id: created.id } }));
    }

    return owners;
};

const folderPathsOf = (folders: ImportSource['folders']) => {
    const byId = new Map(folders.map((folder) => [folder.id, folder]));
    const paths = new Map<string, string[]>();

    const pathOf = (id: string, seen = new Set<string>()): string[] => {
        const folder = byId.get(id);
        if (!folder || seen.has(id)) return [];

        const cached = paths.get(id);
        if (cached) return cached;

        seen.add(id);
        const path = [
            ...(folder.parentId ? pathOf(folder.parentId, seen) : []),
            toFolderName(folder.name),
        ].filter(Boolean);
        paths.set(id, path);

        return path;
    };

    folders.forEach((folder) => pathOf(folder.id));

    return paths;
};

const importFolders = async (event: H3Event, source: ImportSource, owners: Map<string, User>) => {
    const paths = folderPathsOf(source.folders);
    const resolvers = new Map<string, ReturnType<typeof createFolderResolver>>();
    const folderIds = new Map<string, string | null>();

    for (const folder of source.folders.filter((folder) => owners.has(folder.userId))) {
        const owner = owners.get(folder.userId)!;
        const resolver = resolvers.get(owner.id) ?? createFolderResolver(event, owner, true);

        resolvers.set(owner.id, resolver);
        folderIds.set(folder.id, await resolver(paths.get(folder.id)!.slice(0, MAX_FOLDER_DEPTH)));
    }

    return folderIds;
};

const importLink = async (event: H3Event, owner: User, link: ImportSource['links'][number]) => {
    const url = linkUrlSchema.safeParse(link.url);
    const isUsedUp = link.maxClicks > 0 && link.clicks >= link.maxClicks;
    const exists = await prisma.link.count({
        where: { authorId: owner.id, url: link.url, createdAt: link.createdAt },
    });

    if (!url.success || isUsedUp || exists) return false;

    const isCodeFree =
        linkCodeSchema.safeParse(link.code).success &&
        !(await prisma.link.count({ where: { code: link.code } }));

    const created = await prisma.link.create({
        data: {
            url: url.data,
            code: isCodeFree ? link.code : await createLinkCode(),
            password: link.locked ? nanoid(12) : null,
            maxClicks: link.maxClicks,
            clicks: link.clicks,
            createdAt: link.createdAt,
            authorId: owner.id,
        },
    });

    sendToUser(owner.id, 'link:create', serializeLink(event, created, owner.domains));

    return true;
};

const importNote = async (event: H3Event, owner: User, note: ImportSource['notes'][number]) => {
    const title = note.title.trim().slice(0, 48).trim().padEnd(3, '.');
    const exists = await prisma.note.count({
        where: { authorId: owner.id, title, createdAt: note.createdAt },
    });

    if (!note.content || note.content.length > 50_000 || exists) return false;

    const created = await prisma.note.create({
        data: {
            title,
            content: note.content,
            public: note.public,
            createdAt: note.createdAt,
            authorId: owner.id,
        },
    });

    await insert(noteSearchDb, { id: created.id, authorId: owner.id, title });
    sendToUser(owner.id, 'note:create', {
        ...created,
        publicUrl: created.public
            ? buildPublicUrl(event, owner.domains, `/note/${created.id}`)
            : undefined,
    });

    return true;
};

export const runAppImport = async (
    event: H3Event,
    sourceId: string,
    source: ImportSource,
    storageId: string | null,
    owners: Map<string, User>,
) => {
    const progress: StorageImportProgress = {
        storageId,
        app: source.app,
        done: 0,
        total: 0,
        imported: 0,
        skipped: 0,
        missing: 0,
        failed: 0,
    };

    reportImport(progress);

    try {
        const findObject = await createObjectFinder(storageId);
        const folderIds = await importFolders(event, source, owners);
        const ownerOf = (item: { userId: string | null }) =>
            item.userId ? owners.get(item.userId) : undefined;

        const files = source.files.filter(ownerOf);
        const links = source.links.filter(ownerOf);
        const notes = source.notes.filter(ownerOf);
        const now = new Date();

        progress.total = files.length + links.length + notes.length;
        reportImport(progress);

        const runStep = async (label: string, step: () => Promise<boolean | null>) => {
            try {
                const result = await step();
                if (result === null) progress.missing++;
                else if (result) progress.imported++;
                else progress.skipped++;
            } catch (error: any) {
                progress.failed++;
                consola.warn(`Could not import ${label}: ${error.message}`);
            } finally {
                progress.done++;
                reportImport(progress);
            }
        };

        for (const file of files) {
            await runStep(file.path, async () => {
                const owner = ownerOf(file)!;

                if (
                    (file.expiresAt && file.expiresAt <= now) ||
                    (file.maxViews > 0 && file.views >= file.maxViews)
                )
                    return false;

                const found = findObject(file, true);
                if (!found) return null;

                const [key, object] = found;
                const name = toDisplayName(file.name, file.slug);

                if (
                    (await isDuplicateImport(owner, name, object.size)) ||
                    (await exceedsUsableSpace(owner, object.size))
                )
                    return false;

                await importStoredFile(event, owner, storageId, {
                    key,
                    fileName: await availableFileName(name, file.slug),
                    name,
                    size: object.size,
                    mimeType: file.mimeType,
                    createdAt: file.createdAt,
                    folderId: (file.folderId && folderIds.get(file.folderId)) || null,
                    maxViews: file.maxViews,
                    expiresAt: file.expiresAt,
                    password: file.locked ? nanoid(12) : null,
                    views: file.views,
                });

                return true;
            });
        }

        for (const link of links) {
            await runStep(link.code, () => importLink(event, ownerOf(link)!, link));
        }

        for (const note of notes) {
            await runStep(note.title, () => importNote(event, ownerOf(note)!, note));
        }

        if (!storageId) await removeEmptyDirectories(await localImportPath(), true);

        await createLog(event, {
            action: 'Import Files',
            message: `Imported ${progress.imported} of ${progress.total} items from ${
                IMPORT_APPS[source.app]
            } for ${owners.size} users: ${progress.skipped} skipped, ${progress.missing} not found, ${progress.failed} failed`,
        });
    } catch (error: any) {
        consola.error(`App import failed: ${error.message}`);
    } finally {
        await fsp.rm(importSourcePath(sourceId), { force: true });
        reportImport(null);
    }

    return progress;
};
