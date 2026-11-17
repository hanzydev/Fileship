export type UpdaterStatus = {
    version: string;
    url: string;
    state: 'upToDate' | 'available' | 'publishing' | 'failed';
    latestVersion: string;
    updaterAvailable: boolean;
    composeOutdated: boolean;
};

export const useUpdaterStatus = () => {
    return useState<UpdaterStatus | null>('updaterStatus', () => null);
};
