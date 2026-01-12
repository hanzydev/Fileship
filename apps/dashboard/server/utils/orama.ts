import { create } from '@orama/orama';
import { stopwords as englishStopwords } from '@orama/stopwords/english';
import { stopwords as frenchStopwords } from '@orama/stopwords/french';
import { stopwords as germanStopwords } from '@orama/stopwords/german';
import { stopwords as spanishStopwords } from '@orama/stopwords/spanish';
import { stopwords as turkishStopwords } from '@orama/stopwords/turkish';

const stopwords = [
    ...englishStopwords,
    ...frenchStopwords,
    ...germanStopwords,
    ...spanishStopwords,
    ...turkishStopwords,
];

const baseOramaOptions = {
    components: {
        tokenizer: {
            stemming: true,
            stopwords,
        },
    },
};

export const fileSearchDb = create({
    schema: {
        fileName: 'string',
        mimeType: 'string',
        embedding: 'vector[768]',
        textEmbedding: 'vector[384]',
        caption: 'string',
        ocrText: 'string',
    },
    ...baseOramaOptions,
});

export const folderSearchDb = create({
    schema: {
        name: 'string',
    },
    ...baseOramaOptions,
});

export const noteSearchDb = create({
    schema: {
        title: 'string',
    },
    ...baseOramaOptions,
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
