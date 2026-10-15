import { createHash, randomBytes } from 'node:crypto';
import { existsSync, mkdirSync, readdirSync, rmdirSync, statSync } from 'node:fs';
import { basename, join } from 'node:path';

import { app, shell } from 'electron';
import log from 'electron-log/main';
import { io, type Socket } from 'socket.io-client';

import type { DriveStatus } from '../../shared/ipc';
import { handleUnauthorized } from '../account';
import { mimeTypeOf } from '../mime';
import { api, ApiError, desktopUserAgent } from '../api';
import { getSettings, getStore, getToken, updateStore } from '../store';
import { resetUploadSettings, uploadFile } from '../upload';
import { broadcast } from '../windows';
import {
    DriveHelper,
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
    type RemoteFile,
    type RemoteFolder,
} from './paths';

type ApiFile = {
    id: string;
    name: string;
    folderId?: string | null;
    size?: { raw: string } | string | number;
    createdAt: string;
    deletedAt?: string | null;
};

type ApiFolder = { id: string; name: string; createdAt: string; deletedAt?: string | null };

const PAGE_SIZE = 200;
const APPLY_DELAY_MS = 300;
const RETRY_DELAY_MS = 2000;
const REPLACE_WINDOW_MS = 3000;
const RETRY_DELAYS_MS = [5000, 15_000, 60_000];

let helper: DriveHelper | null = null;
let socket: Socket | null = null;
let root = '';
let status: DriveStatus = { state: 'off', root: '', files: 0, folders: 0, error: null };
let queue: Promise<unknown> = Promise.resolve();
let applyTimer: NodeJS.Timeout | null = null;
let isStopping = false;
let isSynced = false;
let lifecycle: Promise<unknown> = Promise.resolve();
let retryTimer: NodeJS.Timeout | null = null;
let retries = 0;

const folders = new Map<string, RemoteFolder>();
const files = new Map<string, RemoteFile>();
const local = new Map<string, { path: string; isFolder: boolean }>();
const pendingPaths = new Set<string>();
const hiddenFiles = new Set<string>();
const cancelledUploads = new Set<string>();

const isPendingIdentity = (identity: string) => identity.startsWith('p:');

const isEmptyFolder = (path: string) => {
    try {
        return readdirSync(path).length === 0;
    } catch {
        return false;
    }
};

export const driveRoot = () => getSettings().drive.folder || join(app.getPath('home'), 'Fileship');

const setStatus = (patch: Partial<DriveStatus>) => {
    status = { ...status, ...patch, files: files.size, folders: folders.size };
    broadcast('drive:status', status);
};

export const getDriveStatus = () => status;

const enqueue = <T>(task: () => Promise<T>) => {
    const run = queue.then(task, task);
    queue = run.catch((error) => log.error('Drive task failed', error));
    return run;
};

const isUnauthorized = (error: unknown) => error instanceof ApiError && error.status === 401;

const driveApi = async <T>(path: string, init?: Parameters<typeof api>[1]) => {
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
});

const toRemoteFolder = (folder: ApiFolder): RemoteFolder => ({
    id: folder.id,
    name: folder.name,
    createdAt: new Date(folder.createdAt).getTime(),
});

const loadAll = async <T>(path: string) => {
    const items: T[] = [];

    for (let offset = 0; ; offset += PAGE_SIZE) {
        const separator = path.includes('?') ? '&' : '?';
        const page = await driveApi<{ items: T[]; total: number }>(
            `${path}${separator}limit=${PAGE_SIZE}&offset=${offset}`,
        );

        items.push(...page.items);
        if (page.items.length < PAGE_SIZE || items.length >= page.total) return items;
    }
};

const loadRemote = async () => {
    const [remoteFolders, remoteFiles] = await Promise.all([
        loadAll<ApiFolder>('/api/folders?sort=createdAt:asc'),
        loadAll<ApiFile>('/api/files?folder=all&sort=createdAt:asc'),
    ]);

    folders.clear();
    files.clear();

    for (const folder of remoteFolders) folders.set(folder.id, toRemoteFolder(folder));
    for (const file of remoteFiles) files.set(file.id, toRemoteFile(file));
};

const folderIdAt = (path: string) => {
    for (const [identity, item] of local) {
        if (item.isFolder && item.path.toLowerCase() === path.toLowerCase())
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
    }

    local.set(entry.identity, { path: entry.path, isFolder: entry.isFolder });
};

const apply = async () => {
    if (!helper?.isRunning) return;

    const layout = buildLayout(
        [...folders.values()],
        [...files.values()].filter((file) => !hiddenFiles.has(file.id)),
    );
    const entries = [...layout.values()];

    for (const entry of entries.filter((item) => item.isFolder)) {
        await placeEntry(entry).catch((error) => log.warn('Could not place', entry.path, error));
    }

    for (const entry of entries.filter((item) => !item.isFolder)) {
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

    setStatus({ state: pendingPaths.size ? 'syncing' : 'ready', error: null });
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
        setStatus({ state: 'syncing' });
        await loadRemote();

        const { items } = await helper!.request<{ items: ScanItem[] }>('scan');
        local.clear();

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
        log.warn('Could not reload the drive', error);
        isSynced = false;
        if (!isUnauthorized(error)) scheduleRetry(error.message);
    });

const isMissing = (error: unknown) => error instanceof ApiError && error.status === 404;

const deleteRemote = async (identity: string, isPermanent = false) => {
    const id = identity.slice(2);
    const isFile = identity.startsWith('f:');

    try {
        if (isFile) {
            await driveApi(`/api/files/${id}`, { method: 'DELETE' });
        } else {
            await driveApi(`/api/folders/${id}`, {
                method: 'DELETE',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ deleteFilesToo: true }),
            });
        }
    } catch (error) {
        if (!isMissing(error)) throw error;
    }

    if (isPermanent) {
        await driveApi('/api/trash/delete', {
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

const handleDelete = async (event: Extract<HelperEvent, { type: 'delete' }>) => {
    if (isPendingIdentity(event.identity)) {
        cancelledUploads.add(event.identity);
        return;
    }

    if (event.isFolder) {
        try {
            await deleteRemote(event.identity, true);
            setStatus({});
        } catch (error) {
            log.warn('Could not delete on Fileship', event.path, error);
            scheduleApply();
        }

        return;
    }

    const id = event.identity.slice(2);
    const absolute = join(root, event.path);

    hiddenFiles.add(id);
    local.delete(event.identity);
    setStatus({});

    setTimeout(() => {
        enqueue(async () => {
            try {
                await deleteRemote(event.identity, !existsSync(absolute));
            } catch (error) {
                log.warn('Could not delete on Fileship', event.path, error);
            } finally {
                hiddenFiles.delete(id);
                setStatus({});
                scheduleApply();
            }
        });
    }, REPLACE_WINDOW_MS);
};

const renameFolder = async (identity: string, from: string, to: string) => {
    const id = identity.slice(2);
    const updated = await driveApi<ApiFolder>(`/api/folders/${id}`, {
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
        const updated = await driveApi<ApiFile>(`/api/files/${id}`, {
            method: 'PATCH',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(body),
        });

        files.set(id, toRemoteFile({ ...updated, folderId }, file));
    }

    local.set(identity, { path: to, isFolder: false });
};

const handleRename = async (event: Extract<HelperEvent, { type: 'rename' }>) => {
    if (isPendingIdentity(event.identity)) {
        helper?.ack(event.requestId, false);
        return;
    }

    try {
        if (event.to === null) {
            await deleteRemote(event.identity);
        } else if (event.isFolder) {
            if (event.to.includes('\\')) throw new Error('Fileship folders cannot be nested');
            await renameFolder(event.identity, event.from, event.to);
        } else {
            await moveFile(event.identity, event.to);
        }

        helper?.ack(event.requestId, true);
        setStatus({});
        scheduleApply();
    } catch (error) {
        log.warn('Could not rename on Fileship', event.from, error);
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
    const uploaded = await uploadFile({
        name: basename(path),
        mimeType: mimeTypeOf(path),
        path: absolute,
        onProgress: (sent, total) => reportProgress(path, sent, total),
        options: {
            folderId,
            fileNameType: 'Random',
            password: '',
            expiration: 0,
            maxViews: 0,
            compression: 0,
        },
    });

    return { id: uploaded.id, name: uploaded.name, folderId, size, createdAt: Date.now() };
};

const startUpload = (
    path: string,
    folderId: string | null,
    finish: (file: RemoteFile) => Promise<void>,
    cleanup: () => void,
) => {
    uploadLocal(path, folderId)
        .then((file) =>
            enqueue(async () => {
                files.set(file.id, file);
                if (helper?.isRunning) await finish(file);
            }),
        )
        .then(
            () => {
                pendingPaths.delete(path);
                cleanup();
                setStatus({});
                scheduleApply();
            },
            (error) => {
                pendingPaths.delete(path);
                cleanup();
                log.warn('Could not upload', path, error);
                setStatus({ state: 'error', error: error.message });
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
            const folder = await driveApi<ApiFolder>('/api/folders', {
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

        setStatus({});
        scheduleApply();
        return;
    }

    if (parts.length > 2) return;

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
        event.path,
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
    );
};

const handleLocalModified = async (event: Extract<HelperEvent, { type: 'localModified' }>) => {
    if (pendingPaths.has(event.path)) return;

    const previousId = event.identity.slice(2);
    const previous = files.get(previousId);

    pendingPaths.add(event.path);
    hiddenFiles.add(previousId);
    setStatus({ state: 'syncing' });

    await helper!
        .request('prepare', { path: event.path, identity: event.identity })
        .catch((error) => log.warn('Could not prepare', event.path, error));

    startUpload(
        event.path,
        previous?.folderId ?? null,
        async (file) => {
            await driveApi(`/api/files/${previousId}`, { method: 'DELETE' }).catch((error) => {
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

const runHelperEvent = (event: Exclude<HelperEvent, { type: 'rescan' }>) => {
    if (event.type === 'delete') return handleDelete(event);
    if (event.type === 'rename') return handleRename(event);
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

    enqueue(() => runHelperEvent(event)).catch((error) => {
        log.warn(`Drive ${event.type} failed`, error);
        setStatus({ state: 'error', error: error.message });
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

    socket.on('file:update', (file: Partial<ApiFile> & { id: string }) => {
        const previous = files.get(file.id);
        if (!previous) return;

        files.set(file.id, {
            ...previous,
            ...(file.name ? { name: file.name } : {}),
            ...('folderId' in file ? { folderId: file.folderId ?? null } : {}),
        });
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

    for (const event of ['folder:delete', 'trash:restore', 'currentUser:update', 'deleteAll']) {
        socket.on(event, () => resync());
    }

    socket.on('settings:update', () => resetUploadSettings());
    socket.io.on('reconnect', () => resync());
};

const syncRootKey = () => {
    const { instanceUrl, user } = getStore();
    return createHash('sha256').update(`${instanceUrl}|${user?.id}`).digest('hex').slice(0, 16);
};

const holdsDrive = async (current: DriveHelper) => {
    const { items } = await current.request<{ items: ScanItem[] }>('scan');
    return items.some((item) => item.identity && !isPendingIdentity(item.identity));
};

const isAllowed = () => {
    const { user, instanceUrl } = getStore();
    return getSettings().drive.enabled && !!user && !!instanceUrl && !!getToken();
};

const clearRetry = () => {
    if (retryTimer) clearTimeout(retryTimer);
    retryTimer = null;
};

const scheduleRetry = (error: string) => {
    clearRetry();
    setStatus({ state: 'error', error });

    const delay = RETRY_DELAYS_MS[Math.min(retries, RETRY_DELAYS_MS.length - 1)]!;
    retries++;
    retryTimer = setTimeout(() => {
        retryTimer = null;
        startDrive();
    }, delay);
};

const serialize = <T>(task: () => Promise<T>) => {
    const run = lifecycle.then(task, task);
    lifecycle = run.catch(() => null);
    return run;
};

const shutDown = async (current: DriveHelper) => {
    isStopping = true;
    isSynced = false;
    socket?.disconnect();
    socket = null;
    await current.stop();
    if (helper === current) helper = null;
};

const launch = async () => {
    const { instanceUrl, siteName } = getStore();
    const current = new DriveHelper();

    root = driveRoot();
    isStopping = false;
    setStatus({ state: 'starting', root, error: null });

    current.on('exit', (code) => {
        if (helper !== current) return;

        helper = null;
        isSynced = false;
        socket?.disconnect();
        socket = null;

        if (!isStopping) {
            log.warn('The drive helper stopped', code);
            scheduleRetry('The drive stopped. It restarts in a few seconds.');
        }
    });

    try {
        const isUnknownRoot =
            getStore().driveRoot !== root && existsSync(root) && !isEmptyFolder(root);

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
        });

        if (isUnknownRoot && !(await holdsDrive(current))) {
            await current.request('unregister').catch(() => null);
            await shutDown(current);
            setStatus({
                state: 'error',
                error: `${root} is not empty. Choose an empty folder for the drive.`,
            });
            return false;
        }

        updateStore({ driveRoot: root });
        current.on('event', handleHelperEvent);
        return true;
    } catch (error: any) {
        log.error('Could not start the drive', error);
        await shutDown(current);
        scheduleRetry(error.message);
        return false;
    }
};

export const startDrive = () =>
    serialize(async () => {
        clearRetry();

        if (!isAllowed() || (helper && isSynced)) return;

        if (!isHelperAvailable()) {
            setStatus({ state: 'error', error: 'The drive helper is missing from this build.' });
            return;
        }

        if (!helper && !(await launch())) return;

        try {
            await reload();
            if (!socket) connectSocket();
            isSynced = true;
            retries = 0;
        } catch (error: any) {
            log.error('Could not load the drive', error);
            if (!isUnauthorized(error)) scheduleRetry(error.message);
        }
    });

export const stopDrive = (options: { remove?: boolean; forget?: boolean } = {}) =>
    serialize(async () => {
        clearRetry();
        retries = 0;

        if (options.forget) updateStore({ driveRoot: null });

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
        files.clear();
        folders.clear();

        if (options.remove) {
            try {
                if (!readdirSync(root).length) rmdirSync(root);
            } catch {
                log.warn('Could not remove the drive folder', root);
            }
        }

        setStatus({ state: 'off', error: null });
    });

export const openDrive = () => shell.openPath(driveRoot());
