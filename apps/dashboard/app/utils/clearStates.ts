export const clearStates = (hard = false, clearUserAndTheme = false) => {
    const logs = useLogs();
    const notes = useNotes();
    const codes = useCodes();
    const files = useFiles();
    const users = useUsers();
    const stats = useStats();
    const embed = useEmbed();
    const domains = useDomains();
    const folders = useFolders();
    const backups = useBackups();
    const sessions = useSessions();
    const passkeys = usePasskeys();
    const currentUser = useAuthUser();
    const currentTheme = useTheme();

    notes.value = [];
    codes.value = [];
    files.value = [];
    folders.value = [];
    stats.value = null;

    currentUser.value!.stats = {
        notes: 0,
        codes: 0,
        files: 0,
        folders: 0,
    };

    if (hard) {
        users.value = [];
        domains.value = [];
        backups.value = [];
        sessions.value = [];
        passkeys.value = [];
        logs.value = { users: [], logs: [] };
        embed.value = defaultEmbed;
    }

    if (clearUserAndTheme) {
        currentUser.value = null;
        currentTheme.value = 'Fileship';
    }
};
