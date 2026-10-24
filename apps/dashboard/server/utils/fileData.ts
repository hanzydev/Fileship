import { rm } from 'node:fs/promises';

import type { H3Event } from 'h3';

import { removeMultiple } from '@orama/orama';

export const removeFileData = async (
    files: { id: string; fileName: string; storageId: string | null }[],
) => {
    await Promise.allSettled(
        files.flatMap((file) => [
            getStorage(file.storageId).then((storage) => storage.remove(uploadKey(file.fileName))),
            rm(getThumbnailPath(file.id), { force: true }),
        ]),
    );

    await removeMultiple(
        fileSearchDb,
        files.map((file) => file.id),
    );

    removeFileVectors(files.map((file) => file.id));
};

export const indexCreatedFile = async (
    event: H3Event,
    owner: { aiSettings?: unknown },
    file: { id: string; authorId: string; fileName: string },
) => {
    if ((owner.aiSettings as any)?.enabled ?? true) event.waitUntil(enqueueFileAnalysis(file));
    event.waitUntil(syncContentHashes());

    await upsertFileSearchEntry(file.id);
};
