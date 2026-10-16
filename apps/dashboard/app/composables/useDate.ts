import dayjs, { type ConfigType } from 'dayjs';

export const useDate = () => {
    const { locale } = useI18n();

    return (value?: ConfigType) => dayjs(value).locale(locale.value);
};
