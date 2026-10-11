export const usePublicSettings = () => {
    return useState<PublicSettings>('publicSettings', () => ({
        siteName: 'Fileship',
        returnHttps: 'auto',
        fileChunkSize: 25,
        registrationEnabled: false,
        turnstileSiteKey: '',
        aiEnabled: false,
    }));
};
