export const fetchFiles = async () => {
    const files = useFiles();
    const data = await $fetch('/api/files');

    files.value = data.map((file) => ({
        ...file,
        expiresAt: file.expiresAt ? new Date(file.expiresAt) : null,
        createdAt: new Date(file.createdAt),
    }));

    return true;
};

export const fetchFolders = async () => {
    const folders = useFolders();
    const data = await $fetch('/api/folders');

    folders.value = data.map((folder) => ({
        ...folder,
        createdAt: new Date(folder.createdAt),
    }));

    return true;
};

export const fetchNotes = async () => {
    const notes = useNotes();
    const data = await $fetch('/api/notes');

    notes.value = data.map((note) => ({
        ...note,
        createdAt: new Date(note.createdAt),
    }));

    return true;
};

export const fetchUsers = async () => {
    const users = useUsers();
    const data = await $fetch('/api/users');

    users.value = data.map((user) => ({
        ...user,
        createdAt: new Date(user.createdAt),
    }));

    return true;
};

export const fetchLogs = async () => {
    const logs = useLogs();
    const data = await $fetch('/api/logs');

    logs.value = {
        users: data.users,
        logs: data.logs.map((log) => ({
            ...log,
            createdAt: new Date(log.createdAt),
        })),
    };

    return true;
};
