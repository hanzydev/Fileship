export const LANGUAGES = ['en', 'tr'] as const;

export type Language = (typeof LANGUAGES)[number];

export const applyLanguage = async (language?: string | null) => {
    const { $i18n } = useNuxtApp();
    const browserLocale = $i18n.getBrowserLocale();

    await $i18n.setLocale(
        (language ||
            (LANGUAGES.includes(browserLocale as Language) ? browserLocale : 'en')) as Language,
    );
};

export const updateLanguage = async (language: Language | null) => {
    const currentUser = useAuthUser();
    const { $toast, $i18n } = useNuxtApp();

    const previousLanguage = currentUser.value!.language;
    if (previousLanguage === language) return;

    currentUser.value!.language = language;
    await applyLanguage(language);

    try {
        await $fetch(`/api/users/${currentUser.value!.id}`, {
            method: 'PATCH',
            body: { language },
        });
    } catch {
        currentUser.value!.language = previousLanguage;
        await applyLanguage(previousLanguage);
        $toast.error($i18n.t('settings.appearance.languageFailed'));
    }
};
