import { create } from '@orama/orama';

export const fileSearchDb = create({
    schema: {
        fileName: 'string',
        mimeType: 'string',
        embedding: 'vector[768]',
    },
});

export const folderSearchDb = create({
    schema: {
        name: 'string',
    },
});

export const noteSearchDb = create({
    schema: {
        title: 'string',
    },
});

export const userSearchDb = create({
    schema: {
        username: 'string',
    },
});

export const logSearchDb = create({
    schema: {
        action: 'string',
        message: 'string',
        ip: 'string',
    },
});
