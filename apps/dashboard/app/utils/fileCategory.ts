import { IFile, IFileArchive, IFileCode, IFileText, IImage, IMusic, IVideo } from '#components';

export const FILE_CATEGORIES = {
    image: { label: 'labels.category.image', icon: IImage },
    video: { label: 'labels.category.video', icon: IVideo },
    audio: { label: 'labels.category.audio', icon: IMusic },
    document: { label: 'labels.category.document', icon: IFileText },
    archive: { label: 'labels.category.archive', icon: IFileArchive },
    code: { label: 'labels.category.code', icon: IFileCode },
    other: { label: 'labels.category.other', icon: IFile },
};

export type FileCategory = keyof typeof FILE_CATEGORIES;

export const getFileCategory = (file: { mimeType: string; fileName: string }): FileCategory => {
    if (file.mimeType.startsWith('image/')) return 'image';
    if (file.mimeType.startsWith('video/')) return 'video';
    if (file.mimeType.startsWith('audio/')) return 'audio';
    if (DOCUMENT_FILE_MIME_TYPES.includes(file.mimeType)) return 'document';
    if (ARCHIVE_FILE_MIME_TYPES.includes(file.mimeType)) return 'archive';
    if (CODE_FILE_EXTENSIONS.includes(getExtname(file.fileName).toLowerCase())) return 'code';
    if (file.mimeType.startsWith('text/')) return 'document';
    return 'other';
};

export const getFileIcon = (file: { mimeType: string; fileName: string }) =>
    FILE_CATEGORIES[getFileCategory(file)].icon;
