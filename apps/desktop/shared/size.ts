import { filesize } from 'filesize';

export const formatStorageSize = (bytes: number, locale: string) =>
    filesize(bytes, { locale, base: 2, standard: 'jedec' });
