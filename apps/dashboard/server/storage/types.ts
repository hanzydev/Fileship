import type { Readable } from 'node:stream';

export interface StoredObject {
    size: number;
    modifiedAt: Date;
}

export type UploadState = Record<string, any>;

export interface StorageDriver {
    put(key: string, localPath: string): Promise<void>;
    read(key: string, range?: { start: number; end: number }): Promise<Readable>;
    download(key: string, localPath: string): Promise<void>;
    move(from: string, to: string): Promise<void>;
    remove(key: string): Promise<void>;
    list(prefix: string): Promise<Map<string, StoredObject>>;
    test(): Promise<void>;
    beginUpload(key: string): Promise<UploadState>;
    writeChunk(
        key: string,
        state: UploadState,
        part: number,
        offset: number,
        data: Uint8Array,
    ): Promise<UploadState>;
    completeUpload(key: string, state: UploadState): Promise<void>;
    abortUpload(key: string, state: UploadState): Promise<void>;
    close?(): Promise<void>;
}

export const notFound = () =>
    createError({
        status: 404,
        message: 'File not found',
    });
