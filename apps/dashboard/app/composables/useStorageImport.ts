import type { ImportApp } from '#shared/utils/importApps';

export interface StorageImportProgress {
    storageId: string | null;
    app: ImportApp | null;
    done: number;
    total: number;
    imported: number;
    skipped: number;
    missing: number;
    failed: number;
}

export const useStorageImport = () =>
    useState<StorageImportProgress | null>('storageImport', () => null);
