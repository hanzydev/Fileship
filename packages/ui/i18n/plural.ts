const russianForm = (count: number) => {
    const ones = count % 10;
    const tens = count % 100;

    if (ones === 1 && tens !== 11) return 0;
    return ones >= 2 && ones <= 4 && (tens < 12 || tens > 14) ? 1 : 2;
};

export const pluralIndex = (locale: string, choice: number, choicesLength: number) => {
    const count = Math.abs(choice);

    if (locale === 'ru' && choicesLength > 2)
        return choicesLength === 4 ? count && russianForm(count) + 1 : russianForm(count);
    if (choicesLength === 2) return Number(locale === 'fr' ? count > 1 : count !== 1);
    return Math.min(count, 2);
};
