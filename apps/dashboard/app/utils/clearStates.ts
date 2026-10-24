export const clearStates = (hard = false, clearUserAndTheme = false) => {
    const currentUser = useAuthUser();

    clearNuxtState('stats');

    if (hard) {
        clearNuxtState('domains');
        clearNuxtState('passkeys');
        clearNuxtState('embed');
        clearNuxtState('storageUsage');
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
