import type { UnwrapNestedRefs } from 'vue';

export type UploadingFileData = File & {
    status?: UnwrapNestedRefs<{
        started: boolean;
        progress: { speed: number; percent: number; eta: number };
        error: string | null;
    }>;
};

export const useUploadingFiles = () => {
    return useState<UploadingFileData[]>('uploadingFiles', () => []);
};

export const useIsUploading = () => {
    return useState<boolean>('isUploading', () => false);
};
