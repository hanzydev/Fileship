import { rm } from 'node:fs/promises';

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
};
