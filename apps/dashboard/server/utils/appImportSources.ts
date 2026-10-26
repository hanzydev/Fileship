import { createReadStream, promises as fsp } from 'node:fs';
import { DatabaseSync } from 'node:sqlite';

import { basename } from 'pathe';
import chain from 'stream-chain';
import { parser } from 'stream-json';
import { ignore } from 'stream-json/filters/ignore.js';
import { streamValues } from 'stream-json/streamers/stream-values.js';

import type { ImportApp } from '#shared/utils/importApps';

export interface ImportSource {
    app: ImportApp;
    version: string | null;
    users: {
        id: string;
        username: string;
        passwordHash: string | null;
        email: string | null;
        usableSpace: number;
    }[];
    folders: {
        id: string;
        userId: string;
        name: string;
        parentId: string | null;
    }[];
    files: {
        id: string;
        userId: string | null;
        path: string;
        slug: string;
        name: string;
        size: number | null;
        mimeType: string | null;
        createdAt: Date;
        views: number;
        maxViews: number;
        expiresAt: Date | null;
        locked: boolean;
        folderId: string | null;
    }[];
    links: {
        userId: string | null;
        code: string;
        url: string;
        clicks: number;
        maxClicks: number;
        createdAt: Date;
        locked: boolean;
    }[];
    notes: {
        userId: string | null;
        title: string;
        content: string;
        public: boolean;
        createdAt: Date;
    }[];
}

type Row = Record<string, any>;

const SQLITE_HEADER = 'SQLite format 3\0';
const IGNORED_EXPORT_PATHS =
    /^(request|stats|thumbnails|thumbnail_map|invites|invite_map|data\.(metrics|thumbnails|invites|userPasskeys|userOauthProviders|userTags))(\.|$)/;
const PLAIN_LANGUAGES = new Set(['', 'plaintext', 'text', 'markdown', 'md']);

const toDate = (value: unknown) => {
    const text = String(value ?? '').trim();
    if (!text) return null;

    const date = /^\d+$/.test(text)
        ? new Date(Number(text) < 1e11 ? Number(text) * 1000 : Number(text))
        : new Date(/(z|[+-]\d\d:?\d\d)$/i.test(text) ? text : `${text.replace(' ', 'T')}Z`);

    return Number.isNaN(date.getTime()) ? null : date;
};

const toCount = (value: unknown) => Math.max(0, Math.trunc(Number(value) || 0));

const toUsableSpace = (bytes: number) =>
    bytes > 0 ? Math.max(1, Math.ceil(bytes / 1024 / 1024)) : -1;

const parseBytes = (value: unknown) => {
    const match = /^\s*([\d.]+)\s*([kmgtp]?)i?b?\s*$/i.exec(String(value ?? ''));
    if (!match) return 0;

    return Number(match[1]) * 1024 ** 'bkmgtp'.indexOf((match[2] || 'b').toLowerCase());
};

const repairLatin1 = (text: string) => {
    if (!/[\u0080-\u00ff]/.test(text) || /[\u0100-\uffff]/.test(text)) return text;

    const decoded = Buffer.from(text, 'latin1').toString('utf8');
    return decoded.includes('\ufffd') ? text : decoded;
};

const keptHash = (password: unknown) =>
    typeof password === 'string' && password.startsWith('$argon2') ? password : null;

const readJson = async (path: string) => {
    for await (const { value } of chain([
        createReadStream(path),
        parser(),
        ignore({ filter: IGNORED_EXPORT_PATHS }),
        streamValues(),
    ]))
        return value as Row;
};

const readZipline4 = (exported: Row): ImportSource => {
    const { data } = exported;
    const quotas = new Map<string, Row>(
        (data.userQuotas ?? []).map((quota: Row) => [quota.userId, quota]),
    );

    return {
        app: 'zipline',
        version: String(exported.versions.zipline),
        users: data.users.map((user: Row) => {
            const quota = quotas.get(user.id);

            return {
                id: String(user.id),
                username: String(user.username),
                passwordHash: keptHash(user.password),
                email: null,
                usableSpace:
                    quota?.filesQuota === 'BY_BYTES'
                        ? toUsableSpace(parseBytes(quota.maxBytes))
                        : -1,
            };
        }),
        folders: (data.folders ?? []).map((folder: Row) => ({
            id: String(folder.id),
            userId: String(folder.userId),
            name: String(folder.name),
            parentId: folder.parentId ? String(folder.parentId) : null,
        })),
        files: data.files.map((file: Row) => ({
            id: String(file.id),
            userId: file.userId ? String(file.userId) : null,
            path: String(file.name),
            slug: String(file.name),
            name: String(file.originalName || file.name),
            size: Number(file.size),
            mimeType: file.type ? String(file.type) : null,
            createdAt: toDate(file.createdAt) ?? new Date(),
            views: toCount(file.views),
            maxViews: toCount(file.maxViews),
            expiresAt: toDate(file.deletesAt),
            locked: !!file.password,
            folderId: file.folderId ? String(file.folderId) : null,
        })),
        links: (data.urls ?? [])
            .filter((url: Row) => url.enabled !== false)
            .map((url: Row) => ({
                userId: url.userId ? String(url.userId) : null,
                code: String(url.vanity || url.code),
                url: String(url.destination),
                clicks: toCount(url.views),
                maxClicks: toCount(url.maxViews),
                createdAt: toDate(url.createdAt) ?? new Date(),
                locked: !!url.password,
            })),
        notes: [],
    };
};

const readZipline3 = (exported: Row): ImportSource => {
    const fileFolders = new Map<string, string>();

    for (const [id, folder] of Object.entries<Row>(exported.folders)) {
        for (const fileId of folder.files) fileFolders.set(String(fileId), id);
    }

    return {
        app: 'zipline',
        version: String(exported.versions.zipline),
        users: Object.entries<Row>(exported.users).map(([id, user]) => ({
            id,
            username: String(user.username),
            passwordHash: keptHash(user.password),
            email: null,
            usableSpace: -1,
        })),
        folders: Object.entries<Row>(exported.folders).map(([id, folder]) => ({
            id,
            userId: String(folder.user),
            name: String(folder.name),
            parentId: null,
        })),
        files: Object.entries<Row>(exported.files).map(([id, file]) => ({
            id,
            userId: file.user ? String(file.user) : null,
            path: String(file.name),
            slug: String(file.name),
            name: file.original_name ? repairLatin1(String(file.original_name)) : String(file.name),
            size: Number(file.size),
            mimeType: file.type ? String(file.type) : null,
            createdAt: toDate(file.created_at) ?? new Date(),
            views: toCount(file.views),
            maxViews: toCount(file.max_views),
            expiresAt: toDate(file.expires_at),
            locked: !!file.password,
            folderId: fileFolders.get(id) ?? null,
        })),
        links: Object.values<Row>(exported.urls).map((url) => ({
            userId: url.user ? String(url.user) : null,
            code: String(url.vanity || url.code),
            url: String(url.destination),
            clicks: toCount(url.views),
            maxClicks: toCount(url.max_views),
            createdAt: toDate(url.created_at) ?? new Date(),
            locked: false,
        })),
        notes: [],
    };
};

const readXBackBone = (db: DatabaseSync): ImportSource => {
    const settings = new Map(
        db
            .prepare('SELECT "key", "value" FROM settings')
            .all()
            .map((row) => [row.key, row.value]),
    );
    const quotaEnabled = settings.get('quota_enabled') === 'on';

    return {
        app: 'xbackbone',
        version: null,
        users: db
            .prepare('SELECT id, username, email, password, max_disk_quota FROM users')
            .all()
            .map((user) => ({
                id: String(user.id),
                username: String(user.username),
                passwordHash: keptHash(user.password),
                email: user.email ? String(user.email) : null,
                usableSpace: quotaEnabled ? toUsableSpace(Number(user.max_disk_quota)) : -1,
            })),
        folders: [],
        files: db
            .prepare(
                'SELECT id, user_id, code, filename, storage_path, published, timestamp FROM uploads',
            )
            .all()
            .map((upload) => ({
                id: String(upload.id),
                userId: upload.user_id === null ? null : String(upload.user_id),
                path: String(upload.storage_path),
                slug: basename(String(upload.storage_path)),
                name: String(upload.filename),
                size: null,
                mimeType: null,
                createdAt: toDate(upload.timestamp) ?? new Date(),
                views: 0,
                maxViews: 0,
                expiresAt: null,
                locked: !upload.published,
                folderId: null,
            })),
        links: [],
        notes: [],
    };
};

const readChibisafe = (db: DatabaseSync): ImportSource => {
    const [settings] = db.prepare('SELECT usersStorageQuota FROM settings LIMIT 1').all();
    const fileAlbums = new Map<string, string>();

    for (const row of db.prepare('SELECT A, B FROM _albumsTofiles ORDER BY A').all()) {
        if (!fileAlbums.has(String(row.B))) fileAlbums.set(String(row.B), String(row.A));
    }

    return {
        app: 'chibisafe',
        version: null,
        users: db
            .prepare('SELECT id, username, password, storageQuota FROM users')
            .all()
            .map((user) => ({
                id: String(user.id),
                username: String(user.username),
                passwordHash: keptHash(user.password),
                email: null,
                usableSpace: toUsableSpace(
                    Number(user.storageQuota) || Number(settings?.usersStorageQuota),
                ),
            })),
        folders: db
            .prepare('SELECT id, userId, name FROM albums')
            .all()
            .map((album) => ({
                id: String(album.id),
                userId: String(album.userId),
                name: String(album.name),
                parentId: null,
            })),
        files: db
            .prepare(
                'SELECT id, userId, name, original, type, size, createdAt FROM files WHERE quarantine = 0',
            )
            .all()
            .map((file) => ({
                id: String(file.id),
                userId: file.userId === null ? null : String(file.userId),
                path: String(file.name),
                slug: String(file.name),
                name: String(file.original || file.name),
                size: Number(file.size),
                mimeType: file.type ? String(file.type) : null,
                createdAt: toDate(file.createdAt) ?? new Date(),
                views: 0,
                maxViews: 0,
                expiresAt: null,
                locked: false,
                folderId: fileAlbums.get(String(file.id)) ?? null,
            })),
        links: db
            .prepare('SELECT userId, identifier, destination, views, createdAt FROM shortenedLinks')
            .all()
            .map((link) => ({
                userId: link.userId === null ? null : String(link.userId),
                code: String(link.identifier),
                url: String(link.destination),
                clicks: toCount(link.views),
                maxClicks: 0,
                createdAt: toDate(link.createdAt) ?? new Date(),
                locked: false,
            })),
        notes: db
            .prepare('SELECT userId, name, content, language, createdAt FROM snippets')
            .all()
            .map((snippet) => {
                const language = String(snippet.language ?? '').toLowerCase();
                const content = String(snippet.content);

                return {
                    userId: snippet.userId === null ? null : String(snippet.userId),
                    title: String(snippet.name),
                    content: PLAIN_LANGUAGES.has(language)
                        ? content
                        : `\`\`\`${language}\n${content}\n\`\`\``,
                    public: true,
                    createdAt: toDate(snippet.createdAt) ?? new Date(),
                };
            }),
    };
};

const readDatabase = (path: string) => {
    const db = new DatabaseSync(path, { readOnly: true });

    try {
        const tables = new Set(
            db
                .prepare("SELECT name FROM sqlite_master WHERE type = 'table'")
                .all()
                .map((row) => row.name),
        );

        if (tables.has('uploads') && tables.has('users')) return readXBackBone(db);
        if (tables.has('albums') && tables.has('snippets')) return readChibisafe(db);
    } finally {
        db.close();
    }
};

const readExport = async (path: string) => {
    const exported = await readJson(path);

    if (exported?.versions?.export === '4') return readZipline4(exported);
    if (exported?.versions?.export === '3') return readZipline3(exported);
};

export const readImportSource = async (path: string) => {
    const handle = await fsp.open(path, 'r');
    const header = Buffer.alloc(SQLITE_HEADER.length);

    try {
        await handle.read(header, 0, header.length, 0);
    } finally {
        await handle.close();
    }

    const source = await (
        header.toString('latin1') === SQLITE_HEADER
            ? Promise.resolve().then(() => readDatabase(path))
            : readExport(path)
    ).catch(() => undefined);

    if (!source) throw apiError(400, 'errorsSystem.importSourceUnknown');

    return source;
};
