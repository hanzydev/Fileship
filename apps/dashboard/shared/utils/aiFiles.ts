export const AI_IMAGE_EXTENSIONS = ['.jpg', '.jpeg', '.png', '.webp'];

export const AI_VIDEO_EXTENSIONS = [
    '.mp4',
    '.webm',
    '.mov',
    '.avi',
    '.mkv',
    '.flv',
    '.wmv',
    '.m4v',
    '.mpg',
    '.mpeg',
    '.3gp',
];

export const AI_TEXT_EXTENSIONS = ['.txt', '.md', '.rtf', '.html', '.htm', '.xml', '.json', '.csv'];

export const getAIFileKind = (fileName: string) => {
    const extension = fileName.slice(fileName.lastIndexOf('.')).toLowerCase();

    if (AI_IMAGE_EXTENSIONS.includes(extension)) return 'image';
    if (AI_VIDEO_EXTENSIONS.includes(extension)) return 'video';
    if (AI_TEXT_EXTENSIONS.includes(extension)) return 'text';
    return null;
};
