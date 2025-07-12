import type themes from '~/styles/themes.json';

export const useTheme = () => {
    return useState<keyof typeof themes>('theme', () => 'Fileship');
};
