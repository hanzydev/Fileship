export const isReducedMotion = () => {
    return window.matchMedia('(prefers-reduced-motion: reduce)').matches;
};
