export default defineNuxtPlugin(() => {
    const colorMode = useColorMode();

    useHead({
        meta: [
            {
                name: 'color-scheme',
                content: () =>
                    colorMode.preference === 'system' ? 'light dark' : colorMode.preference,
                tagPriority: 'critical',
            },
        ],
    });
});
