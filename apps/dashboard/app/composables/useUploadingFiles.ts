import type { UnwrapNestedRefs } from 'vue';

export type UploadingFileData = File & {
    status?: UnwrapNestedRefs<{
        started: boolean;
        completed: boolean;
        progress: { speed: number; percent: number; eta: number };
        error: string | null;
    }>;
    settings?: FileSettings;
};

export const useUploadingFiles = () => {
    return useState<UploadingFileData[]>('uploadingFiles', () => []);
};

export const useIsUploading = () => {
    return useState<boolean>('isUploading', () => false);
};

export const useUploadSheet = () => {
    return useState<boolean>('uploadSheetOpen', () => false);
};
