export const clearStates = (hard = false, clearUserAndTheme = false) => {
    const currentUser = useAuthUser();

    clearNuxtState('notes');
    clearNuxtState('files');
    clearNuxtState('folders');
    clearNuxtState('stats');

    if (hard) {
        clearNuxtState('users');
        clearNuxtState('domains');
        clearNuxtState('backups');
        clearNuxtState('sessions');
        clearNuxtState('passkeys');
        clearNuxtState('logs');
        clearNuxtState('embed');
    }

    if (clearUserAndTheme) {
        clearNuxtState('authUser');
        clearNuxtState('theme');
    } else {
        currentUser.value!.stats = {
            notes: 0,
            files: 0,
            folders: 0,
        };
    }
};
