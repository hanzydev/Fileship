import { gsap } from 'gsap';
import { ScrollTrigger } from 'gsap/ScrollTrigger';
import Lenis from 'lenis';

let lenis: Lenis | null = null;

const update = (time: number) => lenis?.raf(time * 1000);

export const getLenis = () => lenis;

export const prefersReducedMotion = () =>
    window.matchMedia('(prefers-reduced-motion: reduce)').matches;

export const useSmoothScroll = () => {
    onMounted(() => {
        if (prefersReducedMotion()) {
            return;
        }

        lenis = new Lenis({ lerp: 0.18 });
        lenis.on('scroll', ScrollTrigger.update);
        gsap.ticker.add(update);
        gsap.ticker.lagSmoothing(0);
    });

    onBeforeUnmount(() => {
        gsap.ticker.remove(update);
        lenis?.destroy();
        lenis = null;
        ScrollTrigger.getAll().forEach((trigger) => trigger.kill());
    });
};
