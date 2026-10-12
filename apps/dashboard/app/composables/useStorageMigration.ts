export const useStorageMigration = () =>
    useState<{ done: number; total: number } | null>('storageMigration', () => null);
