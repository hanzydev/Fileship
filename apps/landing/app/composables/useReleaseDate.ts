export const useReleaseDate = () => {
    const { locale } = useI18n();

    return (date: string) =>
        new Intl.DateTimeFormat(locale.value, { dateStyle: 'long', timeZone: 'UTC' }).format(
            new Date(date),
        );
};
