export const pluralIndex = (locale: string, choice: number, choicesLength: number) => {
    const count = Math.abs(choice);

    if (choicesLength === 2) return Number(locale === 'fr' ? count > 1 : count !== 1);
    return Math.min(count, 2);
};
