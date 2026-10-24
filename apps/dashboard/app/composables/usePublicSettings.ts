export const usePublicSettings = () => {
    return useState<PublicSettings>('publicSettings', () => ({
        siteName: 'Fileship',
        returnHttps: 'auto',
        fileChunkSize: 25,
        trashRetentionDays: 30,
        scheduledBackupsEnabled: true,
        registrationEnabled: false,
        turnstileSiteKey: '',
        aiEnabled: false,
        uploadConcurrency: 4,
        emailEnabled: false,
    }));
};
