const isSameFile = (a: File, b: File) =>
    a === b || (a.name === b.name && a.size === b.size && a.lastModified === b.lastModified);

export const mergeFiles = <T extends File>(files: T[], added: T[]) =>
    added.reduce(
        (result, file) => (result.some((f) => isSameFile(f, file)) ? result : [...result, file]),
        files,
    );
