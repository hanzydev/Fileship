export const getExtname = (fileName: string) => {
    const match = fileName.match(/\.[^/.]+$/);
    return (match && match[0]) || '';
};

export const getBasename = (fileName: string) => {
    const extname = getExtname(fileName);
    return extname ? fileName.slice(0, -extname.length) : fileName;
};
