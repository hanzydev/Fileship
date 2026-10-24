export type StorageUsage = { used: string; limit: string | null };

export const useStorageUsage = () => {
    return useState<StorageUsage | null>('storageUsage', () => null);
};
