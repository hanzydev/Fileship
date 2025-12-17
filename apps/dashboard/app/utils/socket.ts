import type { Socket } from 'socket.io-client';
import { io } from 'socket.io-client';

let socket: Socket | null;

export const closeSocket = () => {
    if (socket) {
        socket.close();
        socket = null;
    }
};

export const getSocket = () => socket;

export const initSocket = () => {
    const logs = useLogs();
    const notes = useNotes();
    const files = useFiles();
    const users = useUsers();
    const embed = useEmbed();
    const domains = useDomains();
    const folders = useFolders();
    const backups = useBackups();
    const sessions = useSessions();
    const passkeys = usePasskeys();
    const currentUser = useAuthUser();
    const currentTheme = useTheme();
    const runtimeConfig = useRuntimeConfig();

    const route = useRoute();
    const sessionId = useCookie('sessionId');

    if (currentUser.value && sessionId.value) {
        closeSocket();

        socket = io({
            auth: {
                sessionId: sessionId.value,
            },
        });

        socket.on('connect', () => {
            console.log('\x1B[38;2;3;198;252m\x1B[1m[ Socket.IO ]\x1B[0m Connected');
            socket?.emit('client:ready');
        });

        socket.on('disconnect', () => {
            console.log('\x1B[38;2;3;198;252m\x1B[1m[ Socket.IO ]\x1B[0m Disconnected');
        });

        // User
        socket.on('currentUser:update', (data) => {
            const adminRoutes = ['/users', '/logs', '/stats'];
            if (
                data.permissions?.length &&
                !isAdmin(data) &&
                isAdmin(currentUser.value) &&
                adminRoutes.some((r) => route.path.startsWith(r))
            ) {
                navigateTo('/dashboard');
            }

            if (data.theme) currentTheme.value = data.theme;

            currentUser.value = {
                ...currentUser.value,
                ...data,
            };
        });

        socket.on('currentUser:updateTotp', (data) => {
            currentUser.value!.totpEnabled = data;
        });

        socket.on('currentUser:embedUpdate', (data) => {
            embed.value = data;
        });

        socket.on('currentUser:domainsUpdate', (data) => {
            domains.value = data;

            const buildPublicUrl = (route: `/${string}`) => {
                const reqUrl = useRequestURL();

                const protocol = runtimeConfig.public.returnHttps
                    ? runtimeConfig.public.returnHttps === 'true'
                        ? 'https'
                        : 'http'
                    : reqUrl.protocol.slice(0, -1);

                const domain = data.length
                    ? data[Math.floor(Math.random() * data.length)]
                    : reqUrl.host;

                return `${protocol}://${domain}${route}`;
            };

            files.value = files.value.map((file) => ({
                ...file,
                directUrl: buildPublicUrl(`/u/${file.fileName}`),
                embedUrl: buildPublicUrl(`/view/${file.fileName}`),
                thumbnailUrl: file.thumbnailUrl?.length
                    ? buildPublicUrl(`/u/${file.id}/thumbnail`)
                    : file.thumbnailUrl,
            }));

            folders.value = folders.value.map((folder) => ({
                ...folder,
                publicUrl: folder.public ? buildPublicUrl(`/folder/${folder.id}`) : undefined,
            }));
        });

        socket.on('logout', () => {
            clearStates(true, true);

            useCookie('sessionId', { path: '/', sameSite: true }).value = null;
            nextTick(() => {
                navigateTo(`/login?redirectTo=${route.path}`);
            });
        });

        socket.on('deleteAll', clearStates);

        // Sessions
        socket.on('session:create', (data) => {
            sessions.value = [
                {
                    ...data,
                    lastSeen: new Date(data.lastSeen),
                },
                ...sessions.value,
            ];
        });
        socket.on('session:delete', (sessionId) => {
            sessions.value = sessions.value.filter((s) => s.id !== sessionId);
        });
        socket.on('session:deleteAll', () => {
            sessions.value = sessions.value.filter(
                (s) => s.id === currentUser.value?.currentSessionId,
            );
        });

        // Notes
        socket.on('note:create', (data) => {
            notes.value = [
                {
                    ...data,
                    createdAt: new Date(data.createdAt),
                },
                ...notes.value,
            ];
        });

        socket.on('note:update', (data) => {
            const index = notes.value.findIndex((n) => n.id === data.id);

            if (index > -1) {
                notes.value[index] = {
                    ...data,
                    createdAt: new Date(data.createdAt),
                };
            }
        });

        socket.on('note:delete', (noteId) => {
            notes.value = notes.value.filter((n) => n.id !== noteId);
        });

        // Files
        socket.on('file:create', (data) => {
            files.value = [
                {
                    ...data,
                    expiresAt: data.expiresAt ? new Date(data.expiresAt) : null,
                    createdAt: new Date(data.createdAt),
                },
                ...files.value,
            ];

            if (data.folderId) {
                const folder = folders.value.find((f) => f.id === data.folderId);
                if (folder) folder.files.push(data.id);
            }
        });

        socket.on('file:update', (data) => {
            const index = files.value.findIndex((f) => f.id === data.id);

            if (index > -1) {
                const file = files.value[index]!;

                files.value[index] = {
                    ...data,
                    expiresAt: data.expiresAt ? new Date(data.expiresAt) : null,
                    createdAt: new Date(data.createdAt),
                };

                if (file.folderId !== data.folderId) {
                    if (file.folderId) {
                        const folder = folders.value.find((f) => f.id === file.folderId);
                        if (folder) folder.files = folder.files.filter((f) => f !== file.id);
                    }

                    if (data.folderId) {
                        const newFolder = folders.value.find((f) => f.id === data.folderId);
                        if (newFolder) newFolder.files.push(data.id);
                    }
                }
            }
        });

        socket.on('file:delete', (fileId) => {
            const file = files.value.find((f) => f.id === fileId);
            if (!file) return;

            if (file?.folderId) {
                const folder = folders.value.find((f) => f.id === file.folderId);
                if (folder) folder.files = folder.files.filter((f) => f !== fileId);
            }

            files.value = files.value.filter((f) => f.id !== fileId);
        });

        socket.on('file:bulkDelete', (fileIds: string[]) => {
            files.value = files.value.filter((f) => !fileIds.includes(f.id));

            folders.value = folders.value.map((folder) => ({
                ...folder,
                files: folder.files.filter((f) => !fileIds.includes(f)),
            }));
        });

        // Folders
        socket.on('folder:create', (data) => {
            folders.value = [
                {
                    ...data,
                    createdAt: new Date(data.createdAt),
                },
                ...folders.value,
            ];
        });

        socket.on('folder:update', (data) => {
            const index = folders.value.findIndex((f) => f.id === data.id);

            if (index > -1) {
                folders.value[index] = {
                    ...data,
                    createdAt: new Date(data.createdAt),
                };

                files.value = files.value.map((file) => {
                    if (data.files.includes(file.id)) {
                        return {
                            ...file,
                            folderId: data.id,
                        };
                    }

                    if (file.folderId === data.id) {
                        return {
                            ...file,
                            folderId: null,
                        };
                    }

                    return file;
                });
            }
        });

        socket.on('folder:delete', (folderId) => {
            folders.value = folders.value.filter((f) => f.id !== folderId);

            files.value = files.value.map((file) => {
                if (file.folderId === folderId) {
                    return {
                        ...file,
                        folderId: null,
                    };
                }
                return file;
            });
        });

        // Backups
        socket.on('backup:create', (data) => {
            backups.value = [
                {
                    ...data,
                    createdAt: new Date(data.createdAt),
                },
                ...backups.value,
            ];
        });

        socket.on('backup:delete', (backupId) => {
            backups.value = backups.value.filter((b) => b.id !== backupId);
        });

        // Passkeys
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

        // Admin
        socket.on('log:create', (data) => {
            logs.value = {
                logs: [
                    {
                        ...data,
                        user: undefined,
                        userId: data.user?.id,
                        createdAt: new Date(data.createdAt),
                    },
                    ...logs.value.logs,
                ],
                users: [...logs.value.users, data.user].filter(
                    (u, i, self) => u && self.findIndex((t) => t?.id === u?.id) === i,
                ),
            };
        });

        socket.on('log:deleteAll', () => {
            logs.value = {
                users: [],
                logs: [],
            };
        });

        socket.on('user:create', (data) => {
            users.value = [
                {
                    ...data,
                    createdAt: new Date(data.createdAt),
                },
                ...users.value,
            ];
        });

        socket.on('user:delete', (userId) => {
            users.value = users.value.filter((u) => u.id !== userId);
        });

        socket.on('user:update', (data) => {
            const index = users.value.findIndex((u) => u.id === data.id);

            if (data.createdAt) data.createdAt = new Date(data.createdAt);
            if (index > -1) {
                users.value[index] = {
                    ...users.value[index],
                    ...data,
                };
            }
        });

        socket.on('user:updateTotp', (data) => {
            const index = users.value.findIndex((u) => u.id === data.id);
            if (index > -1) {
                users.value[index]!.totpEnabled = data.totpEnabled;
            }
        });
    }
};
