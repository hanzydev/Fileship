export const useTheme = () => {
    return useState<string>('theme', () => 'Fileship');
};

export const updateTheme = async (theme: Accent) => {
    const currentTheme = useTheme();
    const currentUser = useAuthUser();
    const { $toast } = useNuxtApp();

    const previousTheme = currentTheme.value;
    if (previousTheme === theme) return;

    currentTheme.value = theme;

    try {
        await $fetch(`/api/users/${currentUser.value!.id}`, {
            method: 'PATCH',
            body: { theme },
        });
    } catch {
        currentTheme.value = previousTheme;
        $toast.error('Failed to update theme');
    }
};
