import { gsap } from 'gsap';
import { ScrollTrigger } from 'gsap/ScrollTrigger';

export default defineNuxtPlugin((nuxtApp) => {
    if (import.meta.client) {
        gsap.registerPlugin(ScrollTrigger);
    }

    nuxtApp.vueApp.directive<HTMLElement>('reveal', {
        getSSRProps: (binding) => ({ 'data-reveal': binding.modifiers.stagger ? 'stagger' : '' }),
        mounted(el, binding) {
            el.dataset.reveal = binding.modifiers.stagger ? 'stagger' : '';

            if (prefersReducedMotion()) {
                el.dataset.revealDone = '';
                return;
            }

            const targets = binding.modifiers.stagger ? [...el.children] : el;

            gsap.to(targets, {
                opacity: 1,
                y: 0,
                duration: 0.6,
                ease: 'power2.out',
                stagger: binding.modifiers.stagger ? 0.08 : 0,
                scrollTrigger: { trigger: el, start: 'top 85%', once: true },
                onComplete: () => {
                    el.dataset.revealDone = '';
                    gsap.set(targets, { clearProps: 'opacity,transform' });
                },
            });
        },
    });
});
