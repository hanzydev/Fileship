const THEME_COLORS = {
    light: '#ffffff',
    dark: '#0a0a0a',
};

export default defineNuxtPlugin(() => {
    const colorMode = useColorMode();

    useHead({
        meta: (['light', 'dark'] as const).map((scheme) => ({
            name: 'theme-color',
            media: `(prefers-color-scheme: ${scheme})`,
            content: () =>
                colorMode.preference === 'light' || colorMode.preference === 'dark'
                    ? THEME_COLORS[colorMode.preference]
                    : THEME_COLORS[scheme],
        })),
    });
});
