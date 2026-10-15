export type RemoteFolder = { id: string; name: string; createdAt: number };

export type RemoteFile = {
    id: string;
    name: string;
    folderId: string | null;
    size: number;
    createdAt: number;
};

export type LayoutEntry = {
    identity: string;
    path: string;
    isFolder: boolean;
    size: number;
    created: number;
};

const INVALID_CHARACTERS = /[\\/:*?"<>|\p{Cc}]/gu;
const RESERVED_NAMES = /^(con|prn|aux|nul|com\d|lpt\d)(\..*)?$/i;

export const folderIdentity = (id: string) => `d:${id}`;
export const fileIdentity = (id: string) => `f:${id}`;

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

export const buildLayout = (folders: RemoteFolder[], files: RemoteFile[]) => {
    const layout = new Map<string, LayoutEntry>();
    const rootNames = new Set<string>();
    const folderNames = new Map<string, Set<string>>();
    const folderPaths = new Map<string, string>();

    for (const folder of [...folders].sort(byAge)) {
        const path = uniqueName(safeName(folder.name, 'Folder'), rootNames);

        folderPaths.set(folder.id, path);
        folderNames.set(folder.id, new Set());
        layout.set(folderIdentity(folder.id), {
            identity: folderIdentity(folder.id),
            path,
            isFolder: true,
            size: 0,
            created: folder.createdAt,
        });
    }

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

    return layout;
};
