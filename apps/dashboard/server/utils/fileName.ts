import { extname } from 'pathe';

export const safeExtension = (name: string) => {
    const extension = extname(name).replace(/[^a-zA-Z0-9-_.]/g, '');
    return extension.length > 1 ? extension : '';
};

export const assertFileNameAllowed = (fileName: string) => {
    if (isUuid(fileName)) {
        throw createError({
            status: 400,
            message: 'This file name is reserved',
        });
    }
};

export const isValidFileName = (fileName: string) =>
    /^[a-zA-Z0-9-_.]{3,255}$/.test(fileName) && !fileName.startsWith('.') && !isUuid(fileName);
