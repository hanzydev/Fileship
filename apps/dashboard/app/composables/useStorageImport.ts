export interface StorageImportProgress {
    storageId: string | null;
    done: number;
    total: number;
    imported: number;
    skipped: number;
    failed: number;
}

export const useStorageImport = () =>
    useState<StorageImportProgress | null>('storageImport', () => null);
