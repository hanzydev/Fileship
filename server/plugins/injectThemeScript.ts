import themes from '~~/app/styles/themes.json';

export default defineNitroPlugin((nitroApp) => {
    nitroApp.hooks.hook('render:html', (htmlContext) => {
        htmlContext.head.push(
            `<script>
const themes=${JSON.stringify(themes)};

Object.defineProperty(window, 'theme', {
    get() {
        return localStorage.getItem('unsyncedTheme');
    },
    set(themeName) {
        const theme = themes[themeName] || themes.Fileship;

        const cssVariables = [
            ['--fs-background', theme.background],
            ['--fs-accent', theme.accent],
            ...theme.overlays.map((overlay, index) => [\`--fs-overlay-\${index + 1}\`, overlay]),
        ];
        
        for (const [name, value] of cssVariables) {
            document.documentElement.style.setProperty(name, value);
        }

        localStorage.setItem('unsyncedTheme', themeName);
    },
});

window.theme = localStorage.getItem('unsyncedTheme') || 'Fileship';
</script>`,
        );
    });
});
