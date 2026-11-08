import type { RouterConfig } from '@nuxt/schema';

const scrollToHash = (hash: string, smooth: boolean) => {
    const lenis = getLenis();

    if (lenis) {
        lenis.scrollTo(hash, { immediate: !smooth });
        return;
    }

    const element = document.querySelector(hash);

    if (element) {
        const margin = Number.parseFloat(getComputedStyle(element).scrollMarginTop) || 0;
        const top = element.getBoundingClientRect().top + window.scrollY - margin;

        window.scrollTo({ top, behavior: smooth ? 'smooth' : 'instant' });
    }
};

export default {
    scrollBehavior(to, from, savedPosition) {
        if (to.path === from.path && to.hash !== from.hash) {
            if (to.hash) {
                scrollToHash(to.hash, true);
            }

            return false;
        }

        if (to.path === from.path) {
            return false;
        }

        const nuxtApp = useNuxtApp();

        return new Promise((resolve) => {
            nuxtApp.hooks.hookOnce('page:finish', () => {
                if (to.hash && !savedPosition) {
                    resolve({ top: 0 });
                    requestAnimationFrame(() => scrollToHash(to.hash, true));
                    return;
                }

                resolve(savedPosition ?? { top: 0 });
            });
        });
    },
} satisfies RouterConfig;
