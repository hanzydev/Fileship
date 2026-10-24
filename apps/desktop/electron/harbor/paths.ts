export type RemoteFolder = { id: string; name: string; parentId: string | null; createdAt: number };

export type RemoteFile = {
    id: string;
    name: string;
    folderId: string | null;
    size: number;
    createdAt: number;
    isLocked: boolean;
    expiresAt: number | null;
    maxViews: number;
    isSensitive: boolean;
    views: number;
};

export type RemoteNote = {
    id: string;
    title: string;
    size: number;
    hash: string;
    createdAt: number;
};

export type RemoteLink = { id: string; code: string; shortUrl: string; createdAt: number };

export type VirtualFolder<T> = { name: string; items: T[] };

export type LayoutEntry = {
    identity: string;
    path: string;
    isFolder: boolean;
    size: number;
    created: number;
};

const INVALID_CHARACTERS = /[\\/:*?"<>|\p{Cc}]/gu;
const RESERVED_NAMES = /^(con|prn|aux|nul|com\d|lpt\d)(\..*)?$/i;

export const NOTES_FOLDER = 'v:notes';
export const LINKS_FOLDER = 'v:links';

export const folderIdentity = (id: string) => `d:${id}`;
export const fileIdentity = (id: string) => `f:${id}`;
export const noteIdentity = (id: string) => `n:${id}`;
export const linkIdentity = (code: string) => `l:${code}`;

export const linkShortcut = (instanceUrl: string, code: string) =>
    `[InternetShortcut]\r\nURL=${instanceUrl}/s/${code}\r\n`;

export const safeName = (name: string, fallback: string) => {
    let value = name
        .replace(INVALID_CHARACTERS, '')
        .trim()
        .replace(/[. ]+$/, '');

    if (!value) value = fallback;
    if (RESERVED_NAMES.test(value)) value = `_${value}`;

    return value.slice(0, 200);
};

const splitExtension = (name: string) => {
    const dot = name.lastIndexOf('.');
    return dot > 0 ? [name.slice(0, dot), name.slice(dot)] : [name, ''];
};

const uniqueName = (name: string, taken: Set<string>) => {
    const [base, extension] = splitExtension(name);
    let candidate = name;

    for (let index = 2; taken.has(candidate.toLowerCase()); index++) {
        candidate = `${base} (${index})${extension}`;
    }

    taken.add(candidate.toLowerCase());
    return candidate;
};

const byAge = <T extends { id: string; createdAt: number }>(a: T, b: T) =>
    a.createdAt - b.createdAt || a.id.localeCompare(b.id);

const addVirtualFolder = <T extends { id: string; createdAt: number }>(
    layout: Map<string, LayoutEntry>,
    rootNames: Set<string>,
    identity: string,
    folder: VirtualFolder<T> | null,
    entryOf: (
        item: T,
        names: Set<string>,
    ) => Omit<LayoutEntry, 'path' | 'isFolder'> & {
        name: string;
    },
) => {
    if (!folder) return;

    const path = uniqueName(safeName(folder.name, 'Folder'), rootNames);
    const names = new Set<string>();

    layout.set(identity, { identity, path, isFolder: true, size: 0, created: Date.now() });

    for (const item of [...folder.items].sort(byAge)) {
        const { name, ...entry } = entryOf(item, names);
        layout.set(entry.identity, { ...entry, path: `${path}\\${name}`, isFolder: false });
    }
};

export const buildLayout = (
    folders: RemoteFolder[],
    files: RemoteFile[],
    notes: VirtualFolder<RemoteNote> | null = null,
    links: VirtualFolder<RemoteLink> | null = null,
    instanceUrl = '',
) => {
    const layout = new Map<string, LayoutEntry>();
    const rootNames = new Set<string>();
    const folderNames = new Map<string, Set<string>>();
    const folderPaths = new Map<string, string>();
    const known = new Set(folders.map((folder) => folder.id));
    const children = new Map<string | null, RemoteFolder[]>();

    for (const folder of [...folders].sort(byAge)) {
        const parentId = folder.parentId && known.has(folder.parentId) ? folder.parentId : null;
        children.set(parentId, [...(children.get(parentId) ?? []), folder]);
    }

    const addFolders = (parentId: string | null, parentPath: string, names: Set<string>) => {
        for (const folder of children.get(parentId) ?? []) {
            const name = uniqueName(safeName(folder.name, 'Folder'), names);
            const path = parentPath ? `${parentPath}\\${name}` : name;

            folderPaths.set(folder.id, path);
            folderNames.set(folder.id, new Set());
            layout.set(folderIdentity(folder.id), {
                identity: folderIdentity(folder.id),
                path,
                isFolder: true,
                size: 0,
                created: folder.createdAt,
            });

            addFolders(folder.id, path, folderNames.get(folder.id)!);
        }
    };

    addFolders(null, '', rootNames);

    for (const file of [...files].sort(byAge)) {
        const parent = file.folderId ? folderPaths.get(file.folderId) : undefined;
        const names = parent ? folderNames.get(file.folderId!)! : rootNames;
        const name = uniqueName(safeName(file.name, 'File'), names);

        layout.set(fileIdentity(file.id), {
            identity: fileIdentity(file.id),
            path: parent ? `${parent}\\${name}` : name,
            isFolder: false,
            size: file.size,
            created: file.createdAt,
        });
    }

    addVirtualFolder(layout, rootNames, NOTES_FOLDER, notes, (note, names) => ({
        identity: noteIdentity(note.id),
        name: uniqueName(`${safeName(note.title, 'Note')}.md`, names),
        size: note.size,
        created: note.createdAt,
    }));

    addVirtualFolder(layout, rootNames, LINKS_FOLDER, links, (link, names) => ({
        identity: linkIdentity(link.code),
        name: uniqueName(`${safeName(link.code, 'Link')}.url`, names),
        size: Buffer.byteLength(linkShortcut(instanceUrl, link.code)),
        created: link.createdAt,
    }));

    return layout;
};
