import { IFile, IFileArchive, IFileCode, IFileText, IImage, IMusic, IVideo } from '#components';

export const FILE_CATEGORIES = {
    image: { label: 'Images', icon: IImage },
    video: { label: 'Videos', icon: IVideo },
    audio: { label: 'Audio', icon: IMusic },
    document: { label: 'Documents', icon: IFileText },
    archive: { label: 'Archives', icon: IFileArchive },
    code: { label: 'Code', icon: IFileCode },
    other: { label: 'Other', icon: IFile },
};

export type FileCategory = keyof typeof FILE_CATEGORIES;

export const getFileCategory = (file: { mimeType: string; fileName: string }): FileCategory => {
    if (file.mimeType.startsWith('image/')) return 'image';
    if (file.mimeType.startsWith('video/')) return 'video';
    if (file.mimeType.startsWith('audio/')) return 'audio';
    if (DOCUMENT_FILE_MIME_TYPES.includes(file.mimeType)) return 'document';
    if (ARCHIVE_FILE_MIME_TYPES.includes(file.mimeType)) return 'archive';
    if (CODE_FILE_EXTENSIONS.includes(getExtname(file.fileName))) return 'code';
    if (file.mimeType.startsWith('text/')) return 'document';
    return 'other';
};

export const getFileIcon = (file: { mimeType: string; fileName: string }) =>
    FILE_CATEGORIES[getFileCategory(file)].icon;
