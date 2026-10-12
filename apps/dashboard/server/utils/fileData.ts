import { rm } from 'node:fs/promises';

import { join } from 'pathe';

import { removeMultiple } from '@orama/orama';

export const removeFileData = async (files: { id: string; fileName: string }[]) => {
    await Promise.allSettled(
        files.flatMap((file) => [
            rm(join(dataDirectory, 'uploads', file.fileName), { force: true }),
            rm(getThumbnailPath(file.id), { force: true }),
        ]),
    );

    await removeMultiple(
        fileSearchDb,
        files.map((file) => file.id),
    );
};
