export const clearStates = (hard = false, clearUserAndTheme = false) => {
    const logs = useLogs();
    const urls = useUrls();
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

    urls.value = [];
    notes.value = [];
    codes.value = [];
    files.value = [];
    folders.value = [];
    stats.value = null;

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
