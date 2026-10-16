import { basename, extname } from 'pathe';

const FOLDED_LETTERS: Record<string, string> = {
    ı: 'i',
    ł: 'l',
    Ł: 'L',
    ø: 'o',
    Ø: 'O',
    đ: 'd',
    Đ: 'D',
    ð: 'd',
    Ð: 'D',
    ħ: 'h',
    Ħ: 'H',
    þ: 'th',
    Þ: 'Th',
    ß: 'ss',
    æ: 'ae',
    Æ: 'AE',
    œ: 'oe',
    Œ: 'OE',
};

export const toFileNameSlug = (value: string) =>
    value
        .normalize('NFKD')
        .replace(/\p{M}/gu, '')
        .replace(/[^\w.-]/g, (char) => FOLDED_LETTERS[char] ?? '');

export const safeExtension = (name: string) => {
    const extension = toFileNameSlug(extname(name));
    return extension.length > 1 ? extension : '';
};

export const fileNameBase = (name: string) =>
    toFileNameSlug(basename(name, extname(name))).replace(/^\.+/, '');

export const assertFileNameAllowed = (fileName: string) => {
    if (isUuid(fileName)) {
        throw apiError(400, 'errorsFiles.fileNameReserved');
    }
};

export const isValidFileName = (fileName: string) =>
    /^[a-zA-Z0-9-_.]{3,255}$/.test(fileName) && !fileName.startsWith('.') && !isUuid(fileName);
