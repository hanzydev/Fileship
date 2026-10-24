import type { Component } from 'vue';

export const mountApp = (component: Component, title?: string) => {
    const scheme = matchMedia('(prefers-color-scheme: dark)');
    const applyScheme = () => document.documentElement.classList.toggle('dark', scheme.matches);

    applyScheme();
    scheme.addEventListener('change', applyScheme);

    document.documentElement.lang = getLanguage();
    if (title) document.title = title;

    const app = createApp(component);
    app.config.globalProperties.$t = t;
    app.mount('#app');
};
