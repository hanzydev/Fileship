import { createHash, randomBytes } from 'node:crypto';
import {
    existsSync,
    mkdirSync,
    readdirSync,
    readFileSync,
    rmdirSync,
    rmSync,
    statSync,
} from 'node:fs';
import { basename, dirname, extname, join, relative } from 'node:path';
import { setTimeout as sleep } from 'node:timers/promises';

import { app, dialog, shell } from 'electron';
import log from 'electron-log/main';
import { io, type Socket } from 'socket.io-client';

import type {
    HarborActivity,
    HarborActivityKind,
    HarborStatus,
    Permission,
} from '../../shared/ipc';
import { formatStorageSize } from '../../shared/size';
import { relativeTime } from '../../shared/time';
import { handleUnauthorized } from '../account';
import { runAction } from '../actions';
import { api, ApiError, desktopUserAgent, isQuotaExceeded } from '../api';
import { writeText } from '../clipboard';
import { openDashboard } from '../dashboard';
import { getLanguage, t } from '../i18n';
import { mimeTypeOf } from '../mime';
import { setHarborRoot } from '../shell';
import { getSettings, getStore, getToken, updateStore } from '../store';
import { refreshTray } from '../tray';
import { resetUploadSettings, uploadFile } from '../upload';
import { broadcast, notify, openWebUrl, showMainWindow } from '../windows';
import {
    HarborHelper,
    harborSessionPath,
    type HelperEvent,
    helperPath,
    isHelperAvailable,
    type ScanItem,
} from './helper';
import {
    buildLayout,
    fileIdentity,
    folderIdentity,
    type LayoutEntry,
    LINKS_FOLDER,
    noteIdentity,
    NOTES_FOLDER,
    type RemoteFile,
    type RemoteFolder,
    type RemoteLink,
    type RemoteNote,
} from './paths';

type ApiFile = {
    id: string;
    name: string;
    folderId?: string | null;
    size?: { raw: string } | string | number;
    createdAt: string;
    deletedAt?: string | null;
    password?: string | null;
    expiresAt?: string | null;
    maxViews?: number;
    piiDetected?: boolean;
    views?: { total: number };
};

type ApiStorage = { used: string; limit: string | null };

type ApiFolderLink = { public: boolean; publicUrl?: string | null };

type ApiInbox = { id: string; slug: string | null; enabled: boolean; folderId: string };

type ApiFileLinks = {
    mimeType: string;
    directUrl: string;
    embedUrl: string;
    embed: { enabled: boolean };
};

type ApiFolder = { id: string; name: string; createdAt: string; deletedAt?: string | null };

type ApiNote = {
    id: string;
    title: string;
    content: string;
    createdAt: string;
    public?: boolean;
    publicUrl?: string | null;
    deletedAt?: string | null;
};

type ApiLink = { id: string; code: string; shortUrl: string; createdAt: string };

type UploadEvent = Extract<HelperEvent, { type: 'localNew' | 'localModified' }>;

const PAGE_SIZE = 200;
const APPLY_DELAY_MS = 300;
const RETRY_DELAY_MS = 2000;
const REPLACE_WINDOW_MS = 3000;
const RETRY_DELAYS_MS = [5000, 15_000, 60_000];
const FINISH_ATTEMPTS = 3;
const STATES_BATCH = 100;
const STORAGE_DELAY_MS = 2000;
const DETAILS_INTERVAL_MS = 10_000;
const COPY_DELAY_MS = 2000;
const NEARLY_FULL = 0.9;
const ACTIVITY_LIMIT = 50;
const PROPERTY = { locked: 1, expires: 2, sensitive: 3, views: 4 };
const NOTE_EXTENSIONS = ['.md', '.txt'];
const JSON_HEADERS = { 'Content-Type': 'application/json' };

let helper: HarborHelper | null = null;
let socket: Socket | null = null;
let root = '';
let status: HarborStatus = {
    state: 'off',
    root: '',
    files: 0,
    folders: 0,
    error: null,
    storage: null,
    localSize: null,
    lastSyncedAt: null,
};
let isPaused = false;
let isStorageFull = false;
let lastReported = '';
let detailsAt = 0;
let hasStorageApi = true;
let storageTimer: NodeJS.Timeout | null = null;
let copyTimer: NodeJS.Timeout | undefined;
let queue: Promise<unknown> = Promise.resolve();
let applyTimer: NodeJS.Timeout | null = null;
let isStopping = false;
let isSynced = false;
let lifecycle: Promise<unknown> = Promise.resolve();
let retryTimer: NodeJS.Timeout | null = null;
let retries = 0;
let statesQueue: Promise<unknown> = Promise.resolve();

const folders = new Map<string, RemoteFolder>();
const files = new Map<string, RemoteFile>();
const notes = new Map<string, RemoteNote>();
const links = new Map<string, RemoteLink>();
const localHashes = new Map<string, string>();
const replacedNotes = new Set<string>();
const local = new Map<string, { path: string; isFolder: boolean }>();
const pendingPaths = new Set<string>();
const hiddenFiles = new Set<string>();
const cancelledUploads = new Set<string>();
const uploadRetries = new Map<string, number>();
const sentStates = new Map<string, string>();
const activity: HarborActivity[] = [];
const linksToCopy: string[] = [];

const isPendingIdentity = (identity: string) => identity.startsWith('p:');

const linkByCode = (code: string) => [...links.values()].find((link) => link.code === code);

const isKnown = (identity: string) => {
    const id = identity.slice(2);

    if (identity.startsWith('f:')) return files.has(id);
    if (identity.startsWith('n:')) return notes.has(id);
    if (identity.startsWith('l:')) return !!linkByCode(id);
    if (identity.startsWith('v:')) return true;
    return folders.has(id);
};

const can = (permission: Permission) => {
    const { user } = getStore();
    return (
        !!user &&
        (user.superAdmin ||
            user.permissions.includes('Admin') ||
            user.permissions.includes(permission))
    );
};

const hashOf = (content: string) => createHash('sha1').update(content).digest('hex');

const isEmptyFolder = (path: string) => {
    try {
        return readdirSync(path).length === 0;
    } catch {
        return false;
    }
};

export const harborRoot = () =>
    getSettings().harbor.folder || join(app.getPath('home'), 'Fileship');

const formatSize = (bytes: number) => formatStorageSize(bytes, getLanguage());

const isNearlyFull = () => {
    const { storage } = status;
    return !!storage && storage.limit !== null && storage.used >= storage.limit * NEARLY_FULL;
};

const quotaOf = () => {
    if (!status.storage) return null;

    const { used, limit } = status.storage;

    return {
        used,
        total: limit ?? 0,
        label:
            limit === null
                ? t('harbor.quotaUnlimited', { used: formatSize(used) })
                : t('harbor.quotaUsed', { used: formatSize(used), total: formatSize(limit) }),
        isNearlyFull: isNearlyFull(),
    };
};

const summaryOf = () =>
    t('harbor.summary', {
        content: t('harborSettings.contentSummary', {
            files: t('harborSettings.fileCount', { n: files.size, count: files.size }),
            folders: t('harborSettings.folderCount', { n: folders.size, count: folders.size }),
        }),
        local: formatSize(status.localSize ?? 0),
        time: relativeTime(getLanguage(), status.lastSyncedAt!, Date.now(), t('home.justNow')),
    });

const moreInfoOf = () => {
    if (status.state === 'error') {
        return { message: status.error ?? '', action: isStorageFull ? 'storage' : 'retry' };
    }

    if (status.state === 'paused') return { message: t('harbor.pausedMessage'), action: 'resume' };
    if (pendingPaths.size) {
        return { message: t('harbor.uploading', { count: pendingPaths.size }), action: '' };
    }

    if (isNearlyFull()) return { message: t('harbor.almostFull'), action: 'storage' };
    if (status.state !== 'ready' || !status.lastSyncedAt) return { message: '', action: '' };

    return { message: summaryOf(), action: status.localSize ? 'freeUpSpace' : '' };
};

const reportSyncStatus = () => {
    if (!helper?.isRunning) return;

    const payload = {
        error: status.error,
        status: { state: status.state, ...moreInfoOf(), quota: quotaOf() },
    };
    const key = JSON.stringify(payload);
    if (key === lastReported) return;

    lastReported = key;
    helper.request('syncStatus', payload).catch((failure) => {
        lastReported = '';
        log.warn('Could not report the Harbor status', failure);
    });
};

const setStatus = (patch: Partial<HarborStatus>) => {
    const isChanged =
        (!!patch.state && patch.state !== status.state) ||
        (patch.storage !== undefined && patch.storage !== status.storage);

    status = { ...status, ...patch, files: files.size, folders: folders.size };
    broadcast('harbor:status', status);
    if (isChanged) refreshTray();
    reportSyncStatus();
};

const setError = (error: Error) => {
    const isFull = isQuotaExceeded(error);

    if (isFull && !isStorageFull) {
        notify(t('tasks.storageFull'), error.message, () =>
            openDashboard('/dashboard/settings/storage'),
        );
    }

    isStorageFull = isFull;
    setStatus({ state: 'error', error: error.message });
};

const addActivity = (kind: HarborActivityKind, name: string, detail: string | null = null) => {
    activity.unshift({ id: randomBytes(6).toString('hex'), kind, name, detail, at: Date.now() });
    activity.splice(ACTIVITY_LIMIT);
    broadcast('harbor:activity', activity);
};

export const getHarborStatus = () => status;

export const getHarborActivity = () => activity;

export const getHarborStorageLabel = () => quotaOf()?.label ?? null;

const enqueue = <T>(task: () => Promise<T>) => {
    const run = queue.then(task, task);
    queue = run.catch((error) => log.error('Harbor task failed', error));
    return run;
};

const isUnauthorized = (error: unknown) => error instanceof ApiError && error.status === 401;

const harborApi = async <T>(path: string, init?: Parameters<typeof api>[1]) => {
    try {
        return await api<T>(path, init);
    } catch (error) {
        if (isUnauthorized(error)) handleUnauthorized();
        throw error;
    }
};

const toSize = (size: ApiFile['size']) =>
    Number(typeof size === 'object' && size ? size.raw : (size ?? 0)) || 0;

const toRemoteFile = (file: ApiFile, previous?: RemoteFile): RemoteFile => ({
    id: file.id,
    name: file.name ?? previous?.name ?? 'File',
    folderId: file.folderId !== undefined ? file.folderId : (previous?.folderId ?? null),
    size: file.size !== undefined ? toSize(file.size) : (previous?.size ?? 0),
    createdAt: file.createdAt ? new Date(file.createdAt).getTime() : (previous?.createdAt ?? 0),
    isLocked: file.password !== undefined ? !!file.password : (previous?.isLocked ?? false),
    expiresAt:
        file.expiresAt !== undefined
            ? file.expiresAt
                ? new Date(file.expiresAt).getTime()
                : null
            : (previous?.expiresAt ?? null),
    maxViews: file.maxViews ?? previous?.maxViews ?? 0,
    isSensitive: file.piiDetected ?? previous?.isSensitive ?? false,
    views: file.views?.total ?? previous?.views ?? 0,
});

const toRemoteFolder = (folder: ApiFolder): RemoteFolder => ({
    id: folder.id,
    name: folder.name,
    createdAt: new Date(folder.createdAt).getTime(),
});

const toRemoteNote = (note: ApiNote): RemoteNote => ({
    id: note.id,
    title: note.title,
    size: Buffer.byteLength(note.content),
    hash: hashOf(note.content),
    createdAt: new Date(note.createdAt).getTime(),
});

const toRemoteLink = (link: ApiLink): RemoteLink => ({
    id: link.id,
    code: link.code,
    shortUrl: link.shortUrl,
    createdAt: new Date(link.createdAt).getTime(),
});

const loadAll = async <T>(path: string) => {
    const items: T[] = [];

    for (let offset = 0; ; offset += PAGE_SIZE) {
        const separator = path.includes('?') ? '&' : '?';
        const page = await harborApi<{ items: T[]; total: number }>(
            `${path}${separator}limit=${PAGE_SIZE}&offset=${offset}`,
        );

        items.push(...page.items);
        if (page.items.length < PAGE_SIZE || items.length >= page.total) return items;
    }
};

const loadRemote = async () => {
    const [remoteFolders, remoteFiles, remoteNotes, remoteLinks] = await Promise.all([
        loadAll<ApiFolder>('/api/folders?sort=createdAt:asc'),
        loadAll<ApiFile>('/api/files?folder=all&sort=createdAt:asc'),
        can('TakeNotes') ? loadAll<ApiNote>('/api/notes?sort=createdAt:asc') : [],
        can('ShortenLinks') ? loadAll<ApiLink>('/api/links?sort=createdAt:asc') : [],
    ]);

    folders.clear();
    files.clear();
    notes.clear();
    links.clear();

    for (const folder of remoteFolders) folders.set(folder.id, toRemoteFolder(folder));
    for (const file of remoteFiles) files.set(file.id, toRemoteFile(file));
    for (const note of remoteNotes) notes.set(note.id, toRemoteNote(note));
    for (const link of remoteLinks) links.set(link.id, toRemoteLink(link));
};

const folderIdAt = (path: string) => {
    for (const [identity, item] of local) {
        if (
            item.isFolder &&
            identity.startsWith('d:') &&
            item.path.toLowerCase() === path.toLowerCase()
        )
            return identity.slice(2);
    }

    return null;
};

const occupantOf = (path: string) => {
    for (const [identity, item] of local) {
        if (item.path.toLowerCase() === path.toLowerCase()) return identity;
    }

    return null;
};

const parkOccupant = async (path: string, identity: string) => {
    const occupant = occupantOf(path);
    if (!occupant || occupant === identity) return;

    const parked = `${path}.fileship-${Math.random().toString(36).slice(2, 8)}`;
    await helper!.request('move', { from: path, to: parked });
    local.set(occupant, { ...local.get(occupant)!, path: parked });
};

const placeEntry = async (entry: LayoutEntry) => {
    if (pendingPaths.has(entry.path)) return;

    const current = local.get(entry.identity);

    if (current?.path === entry.path) return;

    await parkOccupant(entry.path, entry.identity);

    if (current) {
        await helper!.request('move', { from: current.path, to: entry.path });

        if (entry.isFolder) {
            for (const [identity, item] of local) {
                if (item.path.startsWith(`${current.path}\\`)) {
                    local.set(identity, {
                        ...item,
                        path: entry.path + item.path.slice(current.path.length),
                    });
                }
            }
        }
    } else {
        await helper!.request('create', { items: [entry] });
        sentStates.set(entry.identity, '[]');
    }

    local.set(entry.identity, { path: entry.path, isFolder: entry.isFolder });
};

const formatDate = (time: number) =>
    new Intl.DateTimeFormat(getLanguage(), { dateStyle: 'medium', timeStyle: 'short' }).format(
        time,
    );

const expiryOf = (file: RemoteFile) => {
    if (file.expiresAt) return t('harbor.expiresOn', { date: formatDate(file.expiresAt) });
    return file.maxViews ? t('harbor.expiresAfterViews', { count: file.maxViews }) : null;
};

const propertiesOf = (file: RemoteFile) => {
    const expiry = expiryOf(file);

    return [
        ...(file.isLocked ? [{ id: PROPERTY.locked, value: t('harbor.locked') }] : []),
        ...(expiry ? [{ id: PROPERTY.expires, value: expiry }] : []),
        ...(file.isSensitive ? [{ id: PROPERTY.sensitive, value: t('harbor.sensitive') }] : []),
        ...(file.views
            ? [
                  {
                      id: PROPERTY.views,
                      value: new Intl.NumberFormat(getLanguage()).format(file.views),
                  },
              ]
            : []),
    ];
};

const sendStates = (entries: LayoutEntry[]) => {
    const items = [];

    for (const entry of entries) {
        const file = files.get(entry.identity.slice(2));
        if (entry.isFolder || !file || local.get(entry.identity)?.path !== entry.path) continue;

        const properties = propertiesOf(file);
        const key = JSON.stringify(properties);
        if (sentStates.get(entry.identity) === key) continue;

        sentStates.set(entry.identity, key);
        items.push({ identity: entry.identity, path: entry.path, properties });
    }

    for (let index = 0; index < items.length; index += STATES_BATCH) {
        const batch = items.slice(index, index + STATES_BATCH);
        const current = helper!;

        statesQueue = statesQueue
            .then(() =>
                current.request<{ failed: string[] }>('setStates', {
                    items: batch.map(({ path, properties }) => ({ path, properties })),
                }),
            )
            .then(({ failed }) => {
                for (const item of batch)
                    if (failed.includes(item.path)) sentStates.delete(item.identity);
            })
            .catch((error) => {
                for (const item of batch) sentStates.delete(item.identity);
                log.warn('Could not set the Harbor states', error);
            });
    }
};

const loadStorage = async () => {
    if (!hasStorageApi) return;

    const storage = await harborApi<ApiStorage>('/api/users/@me/storage').then(
        (value) => ({
            used: Number(value.used),
            limit: value.limit === null ? null : Number(value.limit),
        }),
        (error) => {
            if (isMissing(error)) hasStorageApi = false;
            else log.warn('Could not load the storage usage', error);
            return null;
        },
    );

    if (helper?.isRunning) setStatus({ storage });
};

const refreshStorage = () => {
    if (storageTimer) return;

    storageTimer = setTimeout(() => {
        storageTimer = null;
        loadStorage();
    }, STORAGE_DELAY_MS);
};

const refreshDetails = () => {
    if (!helper?.isRunning || Date.now() - detailsAt < DETAILS_INTERVAL_MS) return;

    detailsAt = Date.now();
    refreshStorage();
    helper
        .request<{ local: number }>('usage')
        .then(({ local }) => setStatus({ localSize: local }))
        .catch((error) => log.warn('Could not measure Harbor', error));
};

const apply = async () => {
    if (!helper?.isRunning || isPaused) return;

    const layout = buildLayout(
        [...folders.values()],
        [...files.values()].filter((file) => !hiddenFiles.has(file.id)),
        can('TakeNotes')
            ? {
                  name: t('harbor.notesFolder'),
                  items: [...notes.values()].filter((note) => !hiddenFiles.has(note.id)),
              }
            : null,
        can('ShortenLinks') ? { name: t('harbor.linksFolder'), items: [...links.values()] } : null,
        getStore().instanceUrl!,
    );
    const entries = [...layout.values()];

    for (const entry of entries.filter((item) => item.isFolder)) {
        await placeEntry(entry).catch((error) => log.warn('Could not place', entry.path, error));
    }

    const placed = new Set(
        [...local.values()].filter((item) => item.isFolder).map((item) => item.path),
    );

    for (const entry of entries.filter((item) => !item.isFolder)) {
        const separator = entry.path.indexOf('\\');
        if (separator !== -1 && !placed.has(entry.path.slice(0, separator))) continue;

        await placeEntry(entry).catch((error) => log.warn('Could not place', entry.path, error));
    }

    const stale = [...local.entries()]
        .filter(([identity, item]) => !layout.has(identity) && !pendingPaths.has(item.path))
        .sort(([, a], [, b]) => Number(a.isFolder) - Number(b.isFolder));

    for (const [identity, item] of stale) {
        await helper
            .request('remove', { path: item.path })
            .then(() => local.delete(identity))
            .catch((error) => log.warn('Could not remove', item.path, error));
    }

    sendStates(entries);
    isStorageFull = false;
    setStatus(
        pendingPaths.size
            ? { state: 'syncing', error: null }
            : { state: 'ready', error: null, lastSyncedAt: Date.now() },
    );
    refreshStorage();
};

const scheduleApply = () => {
    if (applyTimer) clearTimeout(applyTimer);
    applyTimer = setTimeout(() => {
        applyTimer = null;
        enqueue(apply);
    }, APPLY_DELAY_MS);
};

const reload = () =>
    enqueue(async () => {
        if (!isPaused) setStatus({ state: 'syncing' });
        await loadRemote();

        const { items } = await helper!.request<{ items: ScanItem[] }>('scan');
        local.clear();
        sentStates.clear();
        status.localSize = items.reduce(
            (sum, item) => sum + (!item.isFolder && item.hydrated ? item.size : 0),
            0,
        );

        const unsynced: ScanItem[] = [];

        for (const item of items) {
            if (!item.identity || isPendingIdentity(item.identity)) {
                unsynced.push({ ...item, identity: null });
            } else if (!item.isFolder && item.modified && item.hydrated) {
                unsynced.push(item);
                if (files.has(item.identity.slice(2))) {
                    local.set(item.identity, { path: item.path, isFolder: false });
                }
            } else {
                local.set(item.identity, { path: item.path, isFolder: item.isFolder });
            }
        }

        await apply();

        const stuck = items.filter(
            (item) =>
                !item.inSync &&
                item.identity &&
                !isPendingIdentity(item.identity) &&
                !item.modified &&
                !pendingPaths.has(item.path) &&
                local.get(item.identity)?.path === item.path &&
                isKnown(item.identity),
        );

        await helper!
            .request('markInSync', { paths: stuck.map((item) => item.path) })
            .catch((error) => log.warn('Could not mark Harbor in sync', error));

        for (const item of unsynced) {
            const event: HelperEvent = item.identity
                ? {
                      type: 'localModified',
                      path: item.path,
                      identity: item.identity,
                      size: item.size,
                  }
                : { type: 'localNew', path: item.path, isFolder: item.isFolder, size: item.size };

            handleHelperEvent(event);
        }
    });

const resync = () =>
    reload().catch((error) => {
        log.warn('Could not reload Harbor', error);
        isSynced = false;
        if (!isUnauthorized(error)) scheduleRetry(error.message);
    });

const isMissing = (error: unknown) => error instanceof ApiError && error.status === 404;

const deleteEntry = async (identity: string, isPermanent: boolean) => {
    const id = identity.slice(2);
    const link = linkByCode(id);
    const isNote = identity.startsWith('n:');

    await harborApi(isNote ? `/api/notes/${id}` : `/api/links/${link?.id}`, {
        method: 'DELETE',
    }).catch((error) => {
        if (!isMissing(error)) throw error;
    });

    if (isNote && isPermanent) {
        await harborApi('/api/trash/delete', {
            method: 'POST',
            headers: JSON_HEADERS,
            body: JSON.stringify({ files: [], folders: [], notes: [id] }),
        });
    }

    notes.delete(id);
    if (link) links.delete(link.id);
    local.delete(identity);
};

const deleteRemote = async (identity: string, isPermanent = false) => {
    if (identity.startsWith('n:') || identity.startsWith('l:')) {
        await deleteEntry(identity, isPermanent);
        return;
    }

    const id = identity.slice(2);
    const isFile = identity.startsWith('f:');

    try {
        if (isFile) {
            await harborApi(`/api/files/${id}`, { method: 'DELETE' });
        } else {
            await harborApi(`/api/folders/${id}`, {
                method: 'DELETE',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ deleteFilesToo: true }),
            });
        }
    } catch (error) {
        if (!isMissing(error)) throw error;
    }

    if (isPermanent) {
        await harborApi('/api/trash/delete', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                files: isFile ? [id] : [],
                folders: isFile ? [] : [id],
                notes: [],
            }),
        });
    }

    if (identity.startsWith('f:')) {
        files.delete(id);
    } else {
        folders.delete(id);
        for (const file of [...files.values()]) if (file.folderId === id) files.delete(file.id);
    }

    const removed = local.get(identity);
    local.delete(identity);

    if (removed?.isFolder) {
        for (const [key, item] of local)
            if (item.path.startsWith(`${removed.path}\\`)) local.delete(key);
    }
};

const noteTitleOf = (path: string) => basename(path, extname(path));

const saveNote = async (identity: string, path: string, isReplaced: boolean) => {
    const id = identity.slice(2);
    const content = readFileSync(join(root, path), 'utf8');
    const note = await harborApi<ApiNote>(`/api/notes/${id}`, {
        method: 'PATCH',
        headers: JSON_HEADERS,
        body: JSON.stringify({ content }),
    });

    notes.set(id, toRemoteNote(note));
    localHashes.set(id, hashOf(content));
    await helper!.request(isReplaced ? 'adopt' : 'update', { path, identity });
    local.set(identity, { path, isFolder: false });
    addActivity('note', basename(path));
};

const createNote = async (event: Extract<HelperEvent, { type: 'localNew' }>) => {
    if (replacedNotes.has(event.path)) return;

    const occupant = occupantOf(event.path);

    if (occupant?.startsWith('n:')) {
        pendingPaths.add(event.path);

        try {
            await saveNote(occupant, event.path, true);
        } finally {
            pendingPaths.delete(event.path);
        }

        setStatus({});
        scheduleApply();
        return;
    }

    if (!NOTE_EXTENSIONS.includes(extname(event.path).toLowerCase())) {
        addActivity('error', basename(event.path), t('harbor.notesOnlyText'));
        return;
    }

    const content = readFileSync(join(root, event.path), 'utf8');
    if (!content) return;

    pendingPaths.add(event.path);

    try {
        const note = await harborApi<ApiNote>('/api/notes', {
            method: 'POST',
            headers: JSON_HEADERS,
            body: JSON.stringify({ title: noteTitleOf(event.path), content }),
        });

        notes.set(note.id, toRemoteNote(note));
        localHashes.set(note.id, hashOf(content));
        await helper!.request('adopt', { path: event.path, identity: noteIdentity(note.id) });
        local.set(noteIdentity(note.id), { path: event.path, isFolder: false });
    } finally {
        pendingPaths.delete(event.path);
    }

    addActivity('note', basename(event.path));
    setStatus({});
    scheduleApply();
};

const updateNote = async (event: Extract<HelperEvent, { type: 'localModified' }>) => {
    if (pendingPaths.has(event.path)) return;

    pendingPaths.add(event.path);

    try {
        await saveNote(event.identity, event.path, false);
    } finally {
        pendingPaths.delete(event.path);
    }

    setStatus({});
};

const refreshNote = (id: string) =>
    enqueue(async () => {
        const note = notes.get(id);
        const entry = local.get(noteIdentity(id));
        if (!note || !entry || !helper?.isRunning || localHashes.get(id) === note.hash) return;

        await helper
            .request<{ refreshed: boolean }>('refresh', { path: entry.path, size: note.size })
            .then(({ refreshed }) => refreshed && localHashes.delete(id))
            .catch((error) => log.warn('Could not refresh the note', entry.path, error));
    });

const handleDelete = async (event: Extract<HelperEvent, { type: 'delete' }>) => {
    if (isPendingIdentity(event.identity)) {
        cancelledUploads.add(event.identity);
        return;
    }

    if (event.isFolder) {
        if (event.identity.startsWith('v:')) {
            local.delete(event.identity);
            scheduleApply();
            return;
        }

        try {
            await deleteRemote(event.identity, true);
            addActivity('delete', basename(event.path));
            setStatus({});
        } catch (error) {
            log.warn('Could not delete on Fileship', event.path, error);
            scheduleApply();
        }

        return;
    }

    const id = event.identity.slice(2);
    const absolute = join(root, event.path);
    const isNote = event.identity.startsWith('n:');

    hiddenFiles.add(id);
    local.delete(event.identity);
    if (isNote) replacedNotes.add(event.path);
    setStatus({});

    setTimeout(() => {
        enqueue(async () => {
            try {
                if (isNote && existsSync(absolute)) {
                    await saveNote(event.identity, event.path, true);
                    return;
                }

                const isPermanent = !existsSync(absolute);

                await deleteRemote(event.identity, isPermanent);
                addActivity(isPermanent ? 'delete' : 'trash', basename(event.path));
            } catch (error) {
                log.warn('Could not delete on Fileship', event.path, error);
            } finally {
                hiddenFiles.delete(id);
                replacedNotes.delete(event.path);
                setStatus({});
                scheduleApply();
            }
        });
    }, REPLACE_WINDOW_MS);
};

const renameFolder = async (identity: string, from: string, to: string) => {
    const id = identity.slice(2);
    const updated = await harborApi<ApiFolder>(`/api/folders/${id}`, {
        method: 'PATCH',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ name: to.slice(0, 32) }),
    });

    folders.set(id, toRemoteFolder(updated));
    local.set(identity, { path: to, isFolder: true });

    for (const [key, item] of local) {
        if (item.path.startsWith(`${from}\\`))
            local.set(key, { ...item, path: to + item.path.slice(from.length) });
    }
};

const moveFile = async (identity: string, to: string) => {
    const id = identity.slice(2);
    const parts = to.split('\\');
    const folderId = parts.length === 2 ? folderIdAt(parts[0]!) : null;

    if (parts.length > 2 || (parts.length === 2 && !folderId)) {
        throw new Error('Fileship folders cannot contain other folders');
    }

    const file = files.get(id);
    const name = parts.at(-1)!;
    const body: Record<string, unknown> = {};

    if (name !== file?.name) body.name = name;
    if (folderId !== (file?.folderId ?? null)) body.folderId = folderId;

    if (Object.keys(body).length) {
        const updated = await harborApi<ApiFile>(`/api/files/${id}`, {
            method: 'PATCH',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(body),
        });

        files.set(id, toRemoteFile({ ...updated, folderId }, file));
    }

    local.set(identity, { path: to, isFolder: false });
};

const renameNote = async (identity: string, to: string) => {
    const id = identity.slice(2);
    const note = await harborApi<ApiNote>(`/api/notes/${id}`, {
        method: 'PATCH',
        headers: JSON_HEADERS,
        body: JSON.stringify({ title: noteTitleOf(to) }),
    });

    notes.set(id, toRemoteNote(note));
    local.set(identity, { path: to, isFolder: false });
};

const isFixedRename = ({ identity, from, to }: Extract<HelperEvent, { type: 'rename' }>) =>
    isPendingIdentity(identity) ||
    identity.startsWith('v:') ||
    (identity.startsWith('l:') && to !== null) ||
    (identity.startsWith('n:') && to !== null && dirname(to) !== dirname(from));

const handleRename = async (event: Extract<HelperEvent, { type: 'rename' }>) => {
    if (isFixedRename(event)) {
        helper?.ack(event.requestId, false);
        return;
    }

    try {
        if (event.to === null) {
            await deleteRemote(event.identity);
            addActivity('trash', basename(event.from));
        } else if (event.identity.startsWith('n:')) {
            await renameNote(event.identity, event.to);
            addActivity('rename', basename(event.to));
        } else if (event.isFolder) {
            if (event.to.includes('\\')) throw new Error('Fileship folders cannot be nested');
            await renameFolder(event.identity, event.from, event.to);
            addActivity('rename', event.to);
        } else {
            await moveFile(event.identity, event.to);
            addActivity(
                dirname(event.to) === dirname(event.from) ? 'rename' : 'move',
                basename(event.to),
            );
        }

        helper?.ack(event.requestId, true);
        setStatus({});
        scheduleApply();
    } catch (error: any) {
        log.warn('Could not rename on Fileship', event.from, error);
        addActivity('error', basename(event.from), error.message);
        helper?.ack(event.requestId, false);
    }
};

const reportProgress = (path: string, sent: number, total: number) =>
    helper
        ?.request('progress', { path, total, completed: sent })
        .catch((error) => log.warn('Could not report progress', path, error));

const uploadLocal = async (path: string, folderId: string | null) => {
    const absolute = join(root, path);
    const size = statSync(absolute).size;
    let reported: Promise<unknown> = Promise.resolve();

    const uploaded = await uploadFile({
        name: basename(path),
        mimeType: mimeTypeOf(path),
        path: absolute,
        onProgress: (sent, total) => {
            reported = reported.then(() => reportProgress(path, sent, total));
        },
        options: {
            folderId,
            fileNameType: 'Random',
            password: '',
            expiration: 0,
            maxViews: 0,
            compression: 0,
        },
    });

    await reported;

    return {
        id: uploaded.id,
        name: uploaded.name,
        folderId,
        size,
        createdAt: Date.now(),
        isLocked: false,
        expiresAt: null,
        maxViews: 0,
        isSensitive: false,
        views: 0,
    };
};

const copyUploadedLinks = async () => {
    const ids = linksToCopy.splice(0);

    try {
        const targets = await Promise.all(ids.map((id) => fileLinksOf(id)));
        const text = targets.map((target) => target.copy).join('\n');

        await writeText(text);
        notify(t('harbor.linkCopied', { count: targets.length }), text, () =>
            openWebUrl(targets[0]!.open),
        );
    } catch (error: any) {
        log.warn('Could not copy the Harbor links', error);
        notify(t('harbor.linkFailed'), error.message);
    }
};

const queueLinkCopy = (id: string) => {
    linksToCopy.push(id);
    clearTimeout(copyTimer);
    copyTimer = setTimeout(copyUploadedLinks, COPY_DELAY_MS);
};

const finishUpload = async (file: RemoteFile, finish: (file: RemoteFile) => Promise<void>) => {
    for (let attempt = 1; ; attempt++) {
        try {
            return await enqueue(async () => {
                files.set(file.id, file);
                if (helper?.isRunning) await finish(file);
            });
        } catch (error) {
            if (attempt >= FINISH_ATTEMPTS || isUnauthorized(error)) throw error;
            await sleep(RETRY_DELAY_MS);
        }
    }
};

const startUpload = (
    event: UploadEvent,
    folderId: string | null,
    finish: (file: RemoteFile) => Promise<void>,
    cleanup: () => void,
    isCancelled = () => false,
) => {
    const { path } = event;

    const release = () => {
        pendingPaths.delete(path);
        cleanup();
    };

    const fail = (error: Error) => {
        uploadRetries.delete(path);
        release();
        log.warn('Could not upload', path, error);
        addActivity('error', basename(path), error.message);
        setError(error);
    };

    uploadLocal(path, folderId).then(
        (file) =>
            finishUpload(file, finish).then(() => {
                const isKept = !isCancelled();

                uploadRetries.delete(path);
                release();

                if (isKept) {
                    addActivity(event.type === 'localNew' ? 'upload' : 'update', basename(path));
                    if (getSettings().harbor.copyLinkOnUpload) queueLinkCopy(file.id);
                }

                setStatus({});
                scheduleApply();
            }, fail),
        (error) => {
            const attempt = uploadRetries.get(path) ?? 0;

            if (
                attempt >= RETRY_DELAYS_MS.length ||
                isUnauthorized(error) ||
                isQuotaExceeded(error) ||
                isCancelled() ||
                !helper?.isRunning ||
                !existsSync(join(root, path))
            ) {
                fail(error);
                return;
            }

            uploadRetries.set(path, attempt + 1);
            release();
            log.warn('Could not upload, trying again', path, error);

            setTimeout(() => {
                if (helper?.isRunning) handleHelperEvent(event);
                else uploadRetries.delete(path);
            }, RETRY_DELAYS_MS[attempt]);
        },
    );
};

const handleLocalNew = async (event: Extract<HelperEvent, { type: 'localNew' }>) => {
    const parts = event.path.split('\\');
    if (pendingPaths.has(event.path) || parts.some((part) => part.includes('.fileship-'))) return;

    if (event.isFolder) {
        if (parts.length !== 1) return;

        pendingPaths.add(event.path);

        try {
            const folder = await harborApi<ApiFolder>('/api/folders', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ name: event.path.slice(0, 32) }),
            });

            folders.set(folder.id, toRemoteFolder(folder));
            await helper!.request('adopt', {
                path: event.path,
                identity: folderIdentity(folder.id),
            });
            local.set(folderIdentity(folder.id), { path: event.path, isFolder: true });
        } finally {
            pendingPaths.delete(event.path);
        }

        addActivity('folder', event.path);
        setStatus({});
        scheduleApply();
        return;
    }

    if (parts.length > 2) return;

    const parent = parts.length === 2 ? occupantOf(parts[0]!) : null;

    if (parent === LINKS_FOLDER) return;
    if (parent === NOTES_FOLDER) return createNote(event);

    const folderId = parts.length === 2 ? folderIdAt(parts[0]!) : null;

    if (parts.length === 2 && !folderId) {
        if (pendingPaths.has(parts[0]!)) {
            setTimeout(() => handleHelperEvent(event), RETRY_DELAY_MS);
        }

        return;
    }

    const occupant = occupantOf(event.path);

    if (occupant?.startsWith('f:')) {
        return handleLocalModified({
            type: 'localModified',
            path: event.path,
            identity: occupant,
            size: event.size,
        });
    }

    pendingPaths.add(event.path);
    setStatus({ state: 'syncing' });

    const generated = `p:${randomBytes(8).toString('hex')}`;
    const prepared = await helper!
        .request<{ identity?: string }>('prepare', { path: event.path, identity: generated })
        .catch((error) => {
            log.warn('Could not prepare', event.path, error);
            return null;
        });

    const pending =
        prepared?.identity && isPendingIdentity(prepared.identity) ? prepared.identity : generated;

    startUpload(
        event,
        folderId,
        async (file) => {
            if (cancelledUploads.has(pending)) {
                await deleteRemote(fileIdentity(file.id), true);
                return;
            }

            await helper!.request('adopt', { path: event.path, identity: fileIdentity(file.id) });
            local.set(fileIdentity(file.id), { path: event.path, isFolder: false });
        },
        () => cancelledUploads.delete(pending),
        () => cancelledUploads.has(pending),
    );
};

const handleLocalModified = async (event: Extract<HelperEvent, { type: 'localModified' }>) => {
    if (event.identity.startsWith('n:')) return updateNote(event);
    if (pendingPaths.has(event.path) || !event.identity.startsWith('f:')) return;

    const previousId = event.identity.slice(2);
    const previous = files.get(previousId);

    pendingPaths.add(event.path);
    hiddenFiles.add(previousId);
    setStatus({ state: 'syncing' });

    await helper!
        .request('prepare', { path: event.path, identity: event.identity })
        .catch((error) => log.warn('Could not prepare', event.path, error));

    startUpload(
        event,
        previous?.folderId ?? null,
        async (file) => {
            await harborApi(`/api/files/${previousId}`, { method: 'DELETE' }).catch((error) => {
                if (!isMissing(error)) throw error;
            });

            files.delete(previousId);
            await helper!.request('update', { path: event.path, identity: fileIdentity(file.id) });

            local.delete(event.identity);
            local.set(fileIdentity(file.id), { path: event.path, isFolder: false });
        },
        () => hiddenFiles.delete(previousId),
    );
};

type SyncEvent = Exclude<HelperEvent, { type: 'rescan' | 'action' | 'statusRequested' }>;

const runHelperEvent = async (event: SyncEvent) => {
    if (event.type === 'delete') return handleDelete(event);
    if (event.type === 'rename') return handleRename(event);
    if (isPaused) return;
    if (event.type === 'localNew') return handleLocalNew(event);
    if (isPendingIdentity(event.identity)) {
        return handleLocalNew({ ...event, type: 'localNew', isFolder: false });
    }

    return handleLocalModified(event);
};

const handleHelperEvent = (event: HelperEvent) => {
    if (event.type === 'rescan') {
        resync();
        return;
    }

    if (event.type === 'action') {
        runHarborAction(event.action, event.paths);
        return;
    }

    if (event.type === 'statusRequested') {
        refreshDetails();
        return;
    }

    enqueue(() => runHelperEvent(event)).catch((error) => {
        log.warn(`Harbor ${event.type} failed`, error);
        addActivity(
            'error',
            basename(event.type === 'rename' ? event.from : event.path),
            error.message,
        );
        setError(error);
    });
};

const connectSocket = () => {
    const { instanceUrl } = getStore();
    const token = getToken();
    if (!instanceUrl || !token) return;

    socket = io(instanceUrl, {
        path: '/socket.io/',
        transports: ['websocket'],
        auth: { token },
        extraHeaders: { 'User-Agent': desktopUserAgent(), Authorization: `Bearer ${token}` },
    });

    socket.on('file:create', (file: ApiFile) => {
        if (file.deletedAt) return;
        files.set(file.id, toRemoteFile(file));
        scheduleApply();
    });

    socket.on('file:update', (file: ApiFile) => {
        const previous = files.get(file.id);
        if (!previous) return;

        files.set(file.id, toRemoteFile(file, previous));
        scheduleApply();
    });

    socket.on('file:delete', (id: string) => {
        files.delete(id);
        scheduleApply();
    });

    socket.on('file:bulkDelete', (ids: string[]) => {
        for (const id of ids) files.delete(id);
        scheduleApply();
    });

    socket.on(
        'file:bulkMove',
        ({ files: ids, folder }: { files: string[]; folder: { id: string } | null }) => {
            for (const id of ids) {
                const file = files.get(id);
                if (file) files.set(id, { ...file, folderId: folder?.id ?? null });
            }
            scheduleApply();
        },
    );

    socket.on('folder:create', (folder: ApiFolder) => {
        folders.set(folder.id, toRemoteFolder(folder));
        scheduleApply();
    });

    socket.on('folder:update', (folder: ApiFolder) => {
        if (folder.deletedAt) folders.delete(folder.id);
        else folders.set(folder.id, toRemoteFolder(folder));
        scheduleApply();
    });

    socket.on('note:create', (note: ApiNote) => {
        if (note.deletedAt) return;
        notes.set(note.id, toRemoteNote(note));
        scheduleApply();
    });

    socket.on('note:update', (note: ApiNote) => {
        const next = toRemoteNote(note);
        const previous = notes.get(note.id);

        notes.set(note.id, next);
        if (previous && previous.hash !== next.hash) refreshNote(note.id);
        scheduleApply();
    });

    socket.on('note:delete', (id: string) => {
        notes.delete(id);
        scheduleApply();
    });

    for (const event of ['link:create', 'link:update']) {
        socket.on(event, (link: ApiLink) => {
            links.set(link.id, toRemoteLink(link));
            scheduleApply();
        });
    }

    socket.on('link:delete', (id: string) => {
        links.delete(id);
        scheduleApply();
    });

    for (const event of ['folder:delete', 'trash:restore', 'currentUser:update', 'deleteAll']) {
        socket.on(event, () => resync());
    }

    socket.on('settings:update', () => resetUploadSettings());
    socket.on('logout', () => handleUnauthorized());
    socket.io.on('reconnect', () => resync());
};

const syncRootKey = () => {
    const { instanceUrl, user } = getStore();
    return createHash('sha256').update(`${instanceUrl}|${user?.id}`).digest('hex').slice(0, 16);
};

const holdsHarbor = async (current: HarborHelper) => {
    const { items } = await current.request<{ items: ScanItem[] }>('scan');
    return items.some((item) => item.identity && !isPendingIdentity(item.identity));
};

const isAllowed = () => {
    const { user, instanceUrl } = getStore();
    return getSettings().harbor.enabled && !!user && !!instanceUrl && !!getToken();
};

const clearRetry = () => {
    if (retryTimer) clearTimeout(retryTimer);
    retryTimer = null;
};

const scheduleRetry = (error: string) => {
    clearRetry();
    isStorageFull = false;
    setStatus({ state: 'error', error });

    const delay = RETRY_DELAYS_MS[Math.min(retries, RETRY_DELAYS_MS.length - 1)]!;
    retries++;
    retryTimer = setTimeout(() => {
        retryTimer = null;
        startHarbor();
    }, delay);
};

const serialize = <T>(task: () => Promise<T>) => {
    const run = lifecycle.then(task, task);
    lifecycle = run.catch(() => null);
    return run;
};

const shutDown = async (current: HarborHelper) => {
    isStopping = true;
    isSynced = false;
    socket?.disconnect();
    socket = null;
    await current.stop();
    if (helper === current) helper = null;
};

const launch = async () => {
    const { instanceUrl, siteName } = getStore();
    const current = new HarborHelper();

    root = harborRoot();
    isStopping = false;
    lastReported = '';
    detailsAt = 0;
    hasStorageApi = true;
    setStatus({ state: 'starting', root, error: null });

    current.on('exit', (code) => {
        if (helper !== current) return;

        helper = null;
        isSynced = false;
        socket?.disconnect();
        socket = null;

        if (!isStopping) {
            log.warn('The Harbor helper stopped', code);
            scheduleRetry(t('harbor.restarting'));
        }
    });

    try {
        const isUnknownRoot =
            getStore().harborRoot !== root && existsSync(root) && !isEmptyFolder(root);

        mkdirSync(root, { recursive: true });
        helper = current;
        current.start();

        await current.request('init', {
            root,
            syncRootId: syncRootKey(),
            displayName: siteName || 'Fileship',
            iconPath: `${helperPath()},0`,
            version: app.getVersion(),
            instanceUrl,
            userAgent: desktopUserAgent(),
            token: getToken(),
            appPath: app.isPackaged ? process.execPath : '',
            properties: [
                { id: PROPERTY.locked, name: t('harbor.locked') },
                { id: PROPERTY.expires, name: t('harbor.expires') },
                { id: PROPERTY.sensitive, name: t('harbor.sensitive') },
                { id: PROPERTY.views, name: t('harbor.views') },
            ],
            labels: {
                copyLink: t('shell.copyLink'),
                upToDate: t('harbor.upToDate'),
                syncing: t('harbor.syncing'),
                paused: t('harbor.paused'),
                error: t('harbor.syncError'),
                offline: t('harbor.offline'),
                offlineMessage: t('harbor.offlineMessage'),
                viewOnline: t('harbor.viewOnline'),
                openApp: t('tray.openFileship'),
                upload: t('harbor.upload'),
                pause: t('harbor.pause'),
                resume: t('harbor.resume'),
                retry: t('harbor.retry'),
                freeUpSpace: t('harbor.freeUpSpace'),
                trash: t('harbor.trash'),
                storage: t('harbor.manageStorage'),
                settings: t('tray.settings'),
            },
        });

        if (isUnknownRoot && !(await holdsHarbor(current))) {
            await current.request('unregister').catch(() => null);
            await shutDown(current);
            setStatus({
                state: 'error',
                error: t('harbor.notEmpty', { root }),
            });
            return false;
        }

        updateStore({ harborRoot: root });
        current.on('event', handleHelperEvent);
        setHarborRoot(root);
        return true;
    } catch (error: any) {
        log.error('Could not start Harbor', error);
        await shutDown(current);
        scheduleRetry(error.message);
        return false;
    }
};

export const startHarbor = () =>
    serialize(async () => {
        clearRetry();

        if (!isAllowed() || (helper && isSynced)) return;

        if (!isHelperAvailable()) {
            setStatus({ state: 'error', error: t('harbor.helperMissing') });
            return;
        }

        if (!helper && !(await launch())) return;

        try {
            await reload();
            if (!socket) connectSocket();
            isSynced = true;
            retries = 0;
        } catch (error: any) {
            log.error('Could not load Harbor', error);
            if (!isUnauthorized(error)) scheduleRetry(error.message);
        }
    });

export const stopHarbor = (options: { remove?: boolean; forget?: boolean } = {}) =>
    serialize(async () => {
        clearRetry();
        retries = 0;
        isPaused = false;
        if (storageTimer) clearTimeout(storageTimer);
        storageTimer = null;

        if (options.remove) setHarborRoot(null);

        if (options.forget) {
            updateStore({ harborRoot: null });
            rmSync(harborSessionPath(), { force: true });
        }

        const current = helper;

        if (!current) {
            if (status.state !== 'off') setStatus({ state: 'off', error: null });
            return;
        }

        isStopping = true;
        isSynced = false;
        socket?.disconnect();
        socket = null;

        if (applyTimer) clearTimeout(applyTimer);
        await queue.catch(() => null);

        if (options.remove) {
            const { items } = await current
                .request<{ items: ScanItem[] }>('scan')
                .catch(() => ({ items: [] as ScanItem[] }));

            const synced = new Set(
                items
                    .filter(
                        (item) =>
                            item.identity &&
                            !isPendingIdentity(item.identity) &&
                            item.inSync &&
                            !item.modified,
                    )
                    .map((item) => item.path),
            );

            const placeholders = [...local.values()]
                .filter((item) => synced.has(item.path))
                .sort((a, b) => Number(a.isFolder) - Number(b.isFolder));

            for (const item of placeholders) {
                if (item.isFolder && !isEmptyFolder(join(root, item.path))) continue;

                await current.request('remove', { path: item.path }).catch((error) => {
                    log.warn('Could not remove', item.path, error);
                });
            }

            await current
                .request('unregister')
                .catch((error) => log.warn('Could not unregister', error));
        }

        await current.stop();
        helper = null;
        local.clear();
        sentStates.clear();
        files.clear();
        folders.clear();
        notes.clear();
        links.clear();
        localHashes.clear();

        if (options.remove) {
            try {
                if (!readdirSync(root).length) rmdirSync(root);
            } catch {
                log.warn('Could not remove the Harbor folder', root);
            }
        }

        setStatus({ state: 'off', error: null, storage: null, localSize: null });
    });

export const pauseHarbor = (paused: boolean) => {
    if (!helper?.isRunning || paused === isPaused) return status;

    isPaused = paused;

    if (paused) {
        setStatus({ state: 'paused' });
    } else {
        setStatus({ state: 'syncing' });
        resync();
    }

    return status;
};

export const freeUpHarborSpace = async () => {
    if (!helper?.isRunning) return 0;

    const { freed } = await helper.request<{ freed: number }>('freeUpSpace', {}, 600_000);

    addActivity('freeUp', formatSize(freed));
    detailsAt = 0;
    refreshDetails();
    return freed;
};

export const openHarbor = () => shell.openPath(harborRoot());

const identityAt = (path: string) => {
    if (!root) return null;

    const target = relative(root, path).toLowerCase();

    for (const [identity, item] of local) {
        if (!item.isFolder && item.path.toLowerCase() === target) return identity;
    }

    return null;
};

const confirmPublic = async (message: string, detail: string) => {
    const { response } = await dialog.showMessageBox({
        type: 'question',
        message,
        detail,
        buttons: [t('harbor.makePublic'), t('harbor.cancel')],
        defaultId: 0,
        cancelId: 1,
    });

    return response === 0;
};

const fileLinksOf = async (id: string) => {
    const file = await harborApi<ApiFileLinks>(`/api/files/${id}`);
    const isPage = file.embed.enabled || file.mimeType.startsWith('text/');
    return { copy: isPage ? file.embedUrl : file.directUrl, open: file.embedUrl };
};

const noteLinksOf = async (id: string) => {
    let note = await harborApi<ApiNote>(`/api/notes/${id}`);

    if (!note.public) {
        if (
            !(await confirmPublic(
                t('harbor.makeNotePublic'),
                t('harbor.makeNotePublicDescription'),
            ))
        )
            return null;

        note = await harborApi<ApiNote>(`/api/notes/${id}`, {
            method: 'PATCH',
            headers: JSON_HEADERS,
            body: JSON.stringify({ public: true }),
        });
    }

    return { copy: note.publicUrl!, open: note.publicUrl! };
};

const linksOf = async (identity: string) => {
    const id = identity.slice(2);

    if (identity.startsWith('n:')) return noteLinksOf(id);
    if (!identity.startsWith('l:')) return fileLinksOf(id);

    const url = linkByCode(id)!.shortUrl;
    return { copy: url, open: url };
};

const dashboardRouteOf = (identity: string) => {
    if (identity.startsWith('n:')) return `/dashboard/notes/${identity.slice(2)}`;
    if (identity.startsWith('l:')) return '/dashboard/links';
    return `/dashboard/files?file=${identity.slice(2)}`;
};

const folderIdOf = (path: string) => (root ? folderIdAt(relative(root, path)) : null);

const copyFolderLink = async (id: string) => {
    let folder = await harborApi<ApiFolderLink>(`/api/folders/${id}`);

    if (!folder.public) {
        if (
            !(await confirmPublic(
                t('harbor.makeFolderPublic'),
                t('harbor.makeFolderPublicDescription'),
            ))
        )
            return;

        await harborApi(`/api/folders/${id}`, {
            method: 'PATCH',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ public: true }),
        });
        folder = await harborApi<ApiFolderLink>(`/api/folders/${id}`);
    }

    const url = folder.publicUrl!;

    await writeText(url);
    notify(t('harbor.folderLinkCopied'), url, () => openWebUrl(url));
};

const copyInboxLink = async (id: string) => {
    const inbox = await harborApi<ApiInbox>('/api/users/@me/inbox');

    if (inbox.folderId !== id || !inbox.enabled) {
        notify(t('harbor.notInbox'), t('harbor.notInboxDescription'), () =>
            openDashboard('/dashboard/settings/inbox'),
        );
        return;
    }

    const url = `${getStore().instanceUrl}/inbox/${inbox.slug || inbox.id}`;

    await writeText(url);
    notify(t('harbor.inboxLinkCopied'), url, () => openWebUrl(url));
};

const runFolderAction = async (action: string, path: string) => {
    const id = folderIdOf(path);

    if (!id) {
        notify(t('harbor.notOnFileship'), t('harbor.notOnFileshipDescription'));
        return;
    }

    try {
        await (action === 'folderLink' ? copyFolderLink(id) : copyInboxLink(id));
    } catch (error: any) {
        log.warn(`Harbor ${action} failed`, error);
        notify(t('harbor.linkFailed'), error.message);
    }
};

const runStatusAction = async (action: string) => {
    if (action === 'pause' || action === 'resume') {
        pauseHarbor(action === 'pause');
        return;
    }

    if (action === 'retry') {
        if (helper?.isRunning) resync();
        return;
    }

    try {
        const freed = await freeUpHarborSpace();
        notify(t('harbor.freedSpace', { size: formatSize(freed) }), harborRoot());
    } catch (error: any) {
        log.warn('Could not free up space', error);
        notify(t('app.somethingWentWrong'), error.message);
    }
};

export const runHarborAction = async (action: string, paths: string[]) => {
    if (action === 'settings') {
        showMainWindow('/settings/harbor');
        return;
    }

    if (action === 'home') {
        showMainWindow();
        return;
    }

    if (action === 'upload') {
        runAction('uploadFile');
        return;
    }

    await startHarbor();

    if (['pause', 'resume', 'retry', 'freeUpSpace'].includes(action)) {
        await runStatusAction(action);
        return;
    }

    if (action === 'folderLink' || action === 'inboxLink') {
        await runFolderAction(action, paths[0] ?? '');
        return;
    }

    const identities = paths
        .map(identityAt)
        .filter((identity) => identity !== null && !isPendingIdentity(identity));

    if (!identities.length) {
        notify(t('harbor.notOnFileship'), t('harbor.notOnFileshipDescription'));
        return;
    }

    if (action === 'show') {
        await openDashboard(dashboardRouteOf(identities[0]!));
        return;
    }

    try {
        const targets = (
            await Promise.all(identities.map((identity) => linksOf(identity!)))
        ).filter((target) => target !== null);

        if (!targets.length) return;

        if (action === 'open') {
            for (const target of targets) openWebUrl(target.open);
            return;
        }

        const text = targets.map((target) => target.copy).join('\n');

        await writeText(text);
        notify(t('harbor.linkCopied', { count: targets.length }), text);
    } catch (error: any) {
        log.warn(`Harbor ${action} failed`, error);
        notify(t('harbor.linkFailed'), error.message);
    }
};

export const harborPathFor = (fileId: string) => {
    const entry = local.get(fileIdentity(fileId));
    return status.state === 'ready' && entry && !entry.isFolder ? join(root, entry.path) : null;
};
