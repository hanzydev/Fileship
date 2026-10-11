export const pluralize = (count: number, word: string, plural = `${word}s`) =>
    `${count} ${count === 1 ? word : plural}`;
