import themes from '~~/app/styles/themes.json';

export default defineNitroPlugin((nitroApp) => {
    nitroApp.hooks.hook('render:html', (htmlContext) => {
        htmlContext.head.push(
            `<script>
let el;

const themes=${JSON.stringify(themes)};

const handleThemeChange = () => {
    if (!el) {
        el = document.createElement('meta');
        el.name = 'theme-color';
        document.head.appendChild(el);
    }

    if (window.matchMedia('(display-mode: standalone)').matches) {
        el.content = themes[window.theme].background;
    } else {
        el.content = themes[window.theme].accent;
    }
};

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
        handleThemeChange();
    },
});

window.theme = localStorage.getItem('unsyncedTheme') || 'Fileship';
window.matchMedia('(display-mode: standalone)').onchange = handleThemeChange;

handleThemeChange();
</script>`,
        );
    });
});
