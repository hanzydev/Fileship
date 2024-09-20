import { io, type Socket } from 'socket.io-client';

import { isAdmin } from '~~/utils/user';

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
    const urls = useUrls();
    const notes = useNotes();
    const codes = useCodes();
    const files = useFiles();
    const users = useUsers();
    const embed = useEmbed();
    const domains = useDomains();
    const folders = useFolders();
    const backups = useBackups();
    const sessions = useSessions();
    const currentUser = useAuthUser();
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
            console.log(
                '\x1B[38;2;3;198;252m\x1B[1m[ Socket.IO ]\x1B[0m Connected',
            );
        });

        socket.on('disconnect', () => {
            console.log(
                '\x1B[38;2;3;198;252m\x1B[1m[ Socket.IO ]\x1B[0m Disconnected',
            );
        });

        // User
        socket.on('update:currentUser', (data) => {
            if (
                !isAdmin(data) &&
                isAdmin(currentUser.value) &&
                route.path.startsWith('/admin')
            ) {
                navigateTo('/dashboard');
            }

            currentUser.value = {
                ...currentUser.value,
                ...data,
            };
        });

        socket.on('update:currentUser:totp', (data) => {
            currentUser.value!.totpEnabled = data;
        });

        socket.on('logout', () => {
            currentUser.value = null;

            useCookie('sessionId', { path: '/', sameSite: true }).value = null;
            nextTick(() => navigateTo(`/login?redirectTo=${route.path}`));
        });

        socket.on('delete:all', () => {
            files.value = [];
            folders.value = [];
            notes.value = [];
            codes.value = [];
            urls.value = [];
        });

        // Sessions
        socket.on('create:session', (data) => {
            sessions.value = [
                {
                    ...data,
                    lastSeen: new Date(data.lastSeen),
                },
                ...sessions.value,
            ];
        });
        socket.on('delete:session', (sessionId) => {
            sessions.value = sessions.value.filter((s) => s.id !== sessionId);
        });
        socket.on('delete:session:all', () => {
            sessions.value = sessions.value.filter(
                (s) => s.id === currentUser.value?.currentSessionId,
            );
        });

        // Notes
        socket.on('create:note', (data) => {
            notes.value = [
                {
                    ...data,
                    createdAt: new Date(data.createdAt),
                },
                ...notes.value,
            ];
        });

        socket.on('update:note', (data) => {
            const index = notes.value.findIndex((n) => n.id === data.id);
            notes.value[index] = {
                ...data,
                createdAt: new Date(data.createdAt),
            };
        });
        socket.on('delete:note', (noteId) => {
            notes.value = notes.value.filter((n) => n.id !== noteId);
        });

        // Urls
        socket.on('create:url', (data) => {
            urls.value = [
                {
                    ...data,
                    expiresAt: data.expiresAt ? new Date(data.expiresAt) : null,
                    createdAt: new Date(data.createdAt),
                },
                ...urls.value,
            ];
        });

        socket.on('update:url', (data) => {
            const index = urls.value.findIndex((u) => u.id === data.id);
            urls.value[index] = {
                ...data,
                expiresAt: data.expiresAt ? new Date(data.expiresAt) : null,
                createdAt: new Date(data.createdAt),
            };
        });

        socket.on('delete:url', (urlId) => {
            urls.value = urls.value.filter((u) => u.id !== urlId);
        });

        // Codes
        socket.on('create:code', (data) => {
            codes.value = [
                {
                    ...data,
                    expiresAt: data.expiresAt ? new Date(data.expiresAt) : null,
                    createdAt: new Date(data.createdAt),
                },
                ...codes.value,
            ];
        });

        socket.on('update:code', (data) => {
            const index = codes.value.findIndex((c) => c.id === data.id);
            codes.value[index] = {
                ...data,
                expiresAt: data.expiresAt ? new Date(data.expiresAt) : null,
                createdAt: new Date(data.createdAt),
            };
        });

        socket.on('delete:code', (codeId) => {
            codes.value = codes.value.filter((c) => c.id !== codeId);
        });

        // Files
        socket.on('create:file', (data) => {
            files.value = [
                {
                    ...data,
                    expiresAt: data.expiresAt ? new Date(data.expiresAt) : null,
                    createdAt: new Date(data.createdAt),
                },
                ...files.value,
            ];
        });

        socket.on('update:file', (data) => {
            const index = files.value.findIndex((f) => f.id === data.id);
            files.value[index] = {
                ...data,
                expiresAt: data.expiresAt ? new Date(data.expiresAt) : null,
                createdAt: new Date(data.createdAt),
            };
        });

        socket.on('delete:file', (fileId) => {
            files.value = files.value.filter((f) => f.id !== fileId);
        });

        // Folders
        socket.on('create:folder', (data) => {
            folders.value = [
                {
                    ...data,
                    createdAt: new Date(data.createdAt),
                },
                ...folders.value,
            ];
        });

        socket.on('update:folder', (data) => {
            const index = folders.value.findIndex((f) => f.id === data.id);
            folders.value[index] = {
                ...data,
                createdAt: new Date(data.createdAt),
            };
        });

        socket.on('delete:folder', (folderId) => {
            folders.value = folders.value.filter((f) => f.id !== folderId);
        });

        socket.on('folder:file:add', ({ folderId, fileId }) => {
            const folderIndex = folders.value.findIndex(
                (f) => f.id === folderId,
            );
            const fileIndex = files.value.findIndex((f) => f.id === fileId);

            if (!folders.value[folderIndex]!.files.find((f) => f === fileId)) {
                folders.value[folderIndex]!.files.push(fileId);
            }

            files.value[fileIndex]!.folderId = folderId;
        });

        socket.on('folder:file:remove', ({ folderId, fileId }) => {
            const folderIndex = folders.value.findIndex(
                (f) => f.id === folderId,
            );
            const fileIndex = files.value.findIndex((f) => f.id === fileId);

            folders.value[folderIndex]!.files = folders.value[
                folderIndex
            ]!.files.filter((f) => f !== fileId);
            files.value[fileIndex]!.folderId = null;
        });

        // Backups
        socket.on('create:backup', (data) => {
            backups.value = [
                {
                    ...data,
                    createdAt: new Date(data.createdAt),
                },
                ...backups.value,
            ];
        });

        socket.on('delete:backup', (backupId) => {
            backups.value = backups.value.filter((b) => b.id !== backupId);
        });

        // Embed
        socket.on('update:embed', (data) => {
            embed.value = data;
        });

        // Domains
        socket.on('update:domains', (data) => {
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
            }));

            urls.value = urls.value.map((url) => ({
                ...url,
                url: buildPublicUrl(`/link/${url.vanity}`),
            }));

            codes.value = codes.value.map((code) => ({
                ...code,
                url: buildPublicUrl(`/code/${code.id}`),
            }));

            folders.value = folders.value.map((folder) => ({
                ...folder,
                publicUrl: folder.public
                    ? buildPublicUrl(`/folder/${folder.id}`)
                    : undefined,
            }));
        });

        // Admin
        socket.on('create:log', (data) => {
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
                    (u, i, self) =>
                        u && self.findIndex((t) => t?.id === u?.id) === i,
                ),
            };
        });

        socket.on('delete:log:all', () => {
            logs.value = {
                users: [],
                logs: [],
            };
        });

        socket.on('create:user', (data) => {
            users.value = [
                {
                    ...data,
                    createdAt: new Date(data.createdAt),
                },
                ...users.value,
            ];
        });

        socket.on('delete:user', (userId) => {
            users.value = users.value.filter((u) => u.id !== userId);
        });

        socket.on('update:user', (data) => {
            const index = users.value.findIndex((u) => u.id === data.id);
            users.value[index] = {
                ...data,
                createdAt: new Date(data.createdAt),
            };
        });

        socket.on('update:user:totp', (data) => {
            const index = users.value.findIndex((u) => u.id === data.id);
            users.value[index]!.totpEnabled = data.totpEnabled;
        });
    }
};
