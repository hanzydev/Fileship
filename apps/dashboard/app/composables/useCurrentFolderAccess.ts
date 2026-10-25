export const useCurrentFolderAccess = () =>
    useState<{ id: string; access: FolderAccess } | null>('currentFolderAccess', () => null);
