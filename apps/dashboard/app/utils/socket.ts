import type { Socket } from 'socket.io-client';
import { io } from 'socket.io-client';

let socket: Socket | null;

const routeAccess: [string, (user: PartialUser) => boolean][] = [
    ['/dashboard/instance', (user) => !!user?.superAdmin],
    ['/dashboard/users', isAdmin],
    ['/dashboard/logs', isAdmin],
    ['/dashboard/stats', isAdmin],
    ['/dashboard/files', canUploadFiles],
    ['/dashboard/folders', canUploadFiles],
    ['/dashboard/notes', canTakeNotes],
    ['/dashboard/links', canShortenLinks],
];

export const closeSocket = () => {
    if (socket) {
        socket.close();
        socket = null;
    }
};

export const getSocket = () => socket;

export const initSocket = () => {
    const embed = useEmbed();
    const domains = useDomains();
    const passkeys = usePasskeys();
    const publicSettings = usePublicSettings();
    const currentUser = useAuthUser();
    const currentTheme = useTheme();
    const storageMigration = useStorageMigration();
    const storageImport = useStorageImport();

    const route = useRoute();

    if (currentUser.value) {
        closeSocket();

        socket = io();

        socket.on('connect', () => socket?.emit('client:ready'));

        socket.on('currentUser:update', (data) => {
            if (data.theme) currentTheme.value = data.theme;

            currentUser.value = {
                ...currentUser.value,
                ...data,
            };

            const access = routeAccess.find(([path]) => route.path.startsWith(path));
            if (access && !access[1](currentUser.value)) navigateTo('/dashboard');
        });

        socket.on('currentUser:updateTotp', (data) => {
            currentUser.value!.totpEnabled = data;
        });

        socket.on('currentUser:embedUpdate', (data) => {
            embed.value = data;
        });

        socket.on('currentUser:domainsUpdate', (data) => {
            domains.value = data;
            refreshLists('files', 'folders', 'notes', 'links');
        });

        socket.on('logout', () => {
            clearStates(true, true);

            nextTick(() => {
                navigateTo(`/login?redirectTo=${route.path}`);
            });
        });

        socket.on('deleteAll', () => {
            clearStates();
            refreshLists('files', 'folders', 'notes', 'backups', 'links', 'trash');
        });

        socket.on('session:create', () => refreshLists('sessions'));
        socket.on('session:delete', (sessionId) => {
            removeListItems('sessions', [sessionId]);
            refreshLists('sessions');
        });
        socket.on('session:deleteAll', () => refreshLists('sessions'));

        socket.on('note:create', () => refreshLists('notes'));
        socket.on('note:update', (data) => patchListItem('notes', data));
        socket.on('note:delete', (noteId) => {
            removeListItems('notes', [noteId]);
            refreshLists('notes');
        });

        socket.on('file:create', () => refreshLists('files', 'folders'));

        socket.on('file:update', (data) => {
            patchListItem('files', data);
            if ('folderId' in data) refreshLists('files', 'folders');
        });

        socket.on('file:delete', (fileId) => {
            removeListItems('files', [fileId]);
            refreshLists('files', 'folders');
        });

        socket.on('file:bulkDelete', (fileIds: string[]) => {
            removeListItems('files', fileIds);
            refreshLists('files', 'folders');
        });

        socket.on('file:bulkMove', ({ files, folder }) => {
            mapListItems('files', (item) =>
                files.includes(item.id) ? { ...item, folderId: folder?.id ?? null, folder } : item,
            );
            refreshLists('files', 'folders');
        });

        socket.on('folder:create', () => refreshLists('folders', 'files'));

        socket.on('folder:update', (data) => {
            patchListItem('folders', data);
            mapListItems('files', (item) =>
                item.folder?.id === data.id
                    ? { ...item, folder: { id: data.id, name: data.name } }
                    : item,
            );
        });

        socket.on('folder:delete', (folderId) => {
            removeListItems('folders', [folderId]);
            refreshLists('folders', 'files');
        });

        socket.on('link:create', () => refreshLists('links'));
        socket.on('link:update', (data) => patchListItem('links', data));
        socket.on('link:delete', (linkId) => {
            removeListItems('links', [linkId]);
            refreshLists('links');
        });

        socket.on('trash:update', () => refreshLists('trash'));
        socket.on('trash:restore', () => refreshLists('files', 'folders', 'notes'));

        socket.on('backup:create', () => refreshLists('backups'));
        socket.on('backup:delete', (backupId) => {
            removeListItems('backups', [backupId]);
            refreshLists('backups');
        });

        socket.on('settings:update', (data) => {
            publicSettings.value = data;
        });

        socket.on('storage:migrate', (data) => {
            storageMigration.value = data;
        });

        socket.on('storage:import', (data) => {
            storageImport.value = data;
        });

        socket.on('passkey:create', (data) => {
            passkeys.value = [...passkeys.value, { ...data, createdAt: new Date(data.createdAt) }];
        });

        socket.on('passkey:delete', (passkeyId) => {
            passkeys.value = passkeys.value.filter((p) => p.id !== passkeyId);
        });

        socket.on('passkey:update', (data) => {
            const index = passkeys.value.findIndex((p) => p.id === data.id);
            if (index > -1) {
                passkeys.value[index] = { ...passkeys.value[index], ...data };
            }
        });

        const refreshLogs = useThrottleFn(() => refreshLists('logs'), 2_000, true);
        socket.on('log:create', refreshLogs);
        socket.on('log:deleteAll', () => refreshLists('logs'));

        socket.on('user:create', () => refreshLists('users'));
        socket.on('user:update', (data) => patchListItem('users', data));
        socket.on('user:updateTotp', (data) => patchListItem('users', data));
        socket.on('user:delete', (userId) => {
            removeListItems('users', [userId]);
            refreshLists('users');
        });
    }
};
