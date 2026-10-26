export const useOidcProvider = () => {
    const { t } = useI18n();
    const publicSettings = usePublicSettings();

    return computed(() => publicSettings.value.oidcButtonLabel || t('auth.oidc.defaultProvider'));
};
