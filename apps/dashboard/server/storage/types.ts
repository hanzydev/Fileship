import type { Readable } from 'node:stream';

export interface StoredObject {
    size: number;
    modifiedAt: Date;
}

export type UploadState = Record<string, any>;

export type UploadPart = Record<string, any>;

export interface StorageDriver {
    put(key: string, localPath: string): Promise<void>;
    read(key: string, range?: { start: number; end: number }): Promise<Readable>;
    download(key: string, localPath: string): Promise<void>;
    move(from: string, to: string): Promise<void>;
    remove(key: string): Promise<void>;
    list(prefix: string): Promise<Map<string, StoredObject>>;
    test(): Promise<void>;
    isSequential?: boolean;
    beginUpload(key: string): Promise<UploadState>;
    writeChunk(
        key: string,
        state: UploadState,
        part: number,
        offset: number,
        data: Uint8Array,
    ): Promise<UploadPart | undefined>;
    completeUpload(key: string, state: UploadState, parts: UploadPart[]): Promise<void>;
    abortUpload(key: string, state: UploadState): Promise<void>;
    close?(): Promise<void>;
}

export const notFound = () => apiError(404, 'errorsSystem.fileNotFound');
