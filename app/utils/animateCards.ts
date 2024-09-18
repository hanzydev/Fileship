import { Cubic, gsap } from 'gsap';

const tweenMap = new Map<gsap.TweenTarget, gsap.core.Tween>();

export const animateCards = () => {
    const enter = (target: gsap.TweenTarget, done?: () => void) => {
        gsap.set(target, { opacity: 1, scale: 1, filter: 'blur(0)' });

        if (isReducedMotion()) return done?.();

        return gsap.from(target, {
            opacity: 0,
            filter: 'blur(0.25rem)',
            scale: 0.95,
            duration: 0.15,
            ease: Cubic.easeOut,
            onComplete: done,
        });
    };

    const leave = (target: gsap.TweenTarget, done?: () => void) => {
        gsap.set(target, { opacity: 1, scale: 1, filter: 'blur(0)' });

        if (isReducedMotion()) return done?.();

        return gsap.to(target, {
            opacity: 0,
            filter: 'blur(0.25rem)',
            scale: 0.95,
            duration: 0.15,
            ease: Cubic.easeOut,
            onComplete: done,
        });
    };

    const all = (id: string, target: gsap.TweenTarget, done?: () => void) => {
        gsap.set(target, { opacity: 1, x: 0, filter: 'blur(0)' });

        if (isReducedMotion()) return done?.();

        const oldTween = tweenMap.get(id);
        if (oldTween) oldTween.kill();

        const tween = gsap.from(target, {
            opacity: 0,
            x: -10,
            filter: 'blur(0.25rem)',
            duration: 0.3,
            stagger: 0.05,
            ease: Cubic.easeOut,
            onComplete: () => {
                tweenMap.delete(id);
                done?.();
            },
        });

        tweenMap.set(id, tween);

        return tween;
    };

    return {
        enter,
        leave,
        all,
    };
};
