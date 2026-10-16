import { create, type Tokenizer } from '@orama/orama';
import { tokenizer as oramaTokenizer } from '@orama/orama/components';
import { stopwords as englishStopwords } from '@orama/stopwords/english';
import { stopwords as frenchStopwords } from '@orama/stopwords/french';
import { stopwords as germanStopwords } from '@orama/stopwords/german';
import { stopwords as spanishStopwords } from '@orama/stopwords/spanish';
import { stopwords as turkishStopwords } from '@orama/stopwords/turkish';

const englishStemmer = oramaTokenizer.createTokenizer({ stemming: true }).stemmer!;
const wordSegmenter = new Intl.Segmenter(undefined, { granularity: 'word' });

const FOLDED_LETTERS: Record<string, string> = {
    ı: 'i',
    ß: 'ss',
    ø: 'o',
    đ: 'd',
    ł: 'l',
    ς: 'σ',
};

const CJK = '\\p{scx=Han}\\p{scx=Hiragana}\\p{scx=Katakana}';
const SOUTHEAST_ASIAN = '\\p{sc=Thai}\\p{sc=Lao}\\p{sc=Khmer}\\p{sc=Myanmar}';
const SEGMENTS = new RegExp(
    `([${CJK}]+)|([${SOUTHEAST_ASIAN}]+)|[^${CJK}${SOUTHEAST_ASIAN}]+`,
    'gu',
);

const fold = (text: string) =>
    text
        .toLowerCase()
        .normalize('NFKD')
        .replace(
            /([\p{sc=Latin}\p{sc=Greek}\p{sc=Cyrillic}\p{sc=Arabic}\p{sc=Hebrew}])\p{M}+/gu,
            '$1',
        )
        .normalize('NFC')
        .replace(/[ıßøđłς]/g, (letter) => FOLDED_LETTERS[letter]!);

const toBigrams = (run: string, withLast: boolean) => {
    const chars = [...run];
    if (chars.length === 1) return chars;

    const bigrams = chars.slice(1).map((char, index) => chars[index] + char);
    return withLast ? [...bigrams, chars.at(-1)!] : bigrams;
};

const createSearchTokenizer = ({
    stemming = false,
    stopWords = [],
}: { stemming?: boolean; stopWords?: string[] } = {}): Tokenizer => {
    const stopWordSet = new Set(stopWords.map(fold));

    const normalize = (token: string) => {
        if (stopWordSet.has(token)) return '';
        return stemming && /^[a-z]+$/.test(token) ? englishStemmer(token) : token;
    };

    return {
        language: 'english',
        normalizationCache: new Map(),
        tokenize: (raw, _language, prop) => {
            const tokens = fold(raw)
                .split(/[^\p{L}\p{M}\p{N}]+/u)
                .flatMap((word) =>
                    [...word.matchAll(SEGMENTS)].flatMap(([segment, cjk, southeastAsian]) => {
                        if (cjk) return toBigrams(cjk, prop !== undefined);
                        if (southeastAsian)
                            return [...wordSegmenter.segment(southeastAsian)]
                                .filter((part) => part.isWordLike)
                                .map((part) => part.segment);
                        return [segment];
                    }),
                )
                .map(normalize)
                .filter(Boolean);

            return [...new Set(tokens)];
        },
    };
};

const textTokenizer = createSearchTokenizer({
    stemming: true,
    stopWords: [
        ...englishStopwords,
        ...frenchStopwords,
        ...germanStopwords,
        ...spanishStopwords,
        ...turkishStopwords,
    ],
});

const baseOramaOptions = {
    components: {
        tokenizer: textTokenizer,
    },
};

export const fileSearchDb = create({
    schema: {
        authorId: 'enum',
        fileName: 'string',
        mimeType: 'string',
        caption: 'string',
        ocrText: 'string',
    },
    ...baseOramaOptions,
});

export const folderSearchDb = create({
    schema: {
        authorId: 'enum',
        name: 'string',
    },
    ...baseOramaOptions,
});

export const noteSearchDb = create({
    schema: {
        authorId: 'enum',
        title: 'string',
    },
    ...baseOramaOptions,
});

export const userSearchDb = create({
    schema: {
        username: 'string',
    },
    components: {
        tokenizer: createSearchTokenizer(),
    },
});

export const logSearchDb = create({
    schema: {
        action: 'string',
        message: 'string',
        ip: 'string',
    },
    components: {
        tokenizer: createSearchTokenizer(),
    },
});
