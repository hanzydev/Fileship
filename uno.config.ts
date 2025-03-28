import {
    defineConfig,
    presetAttributify,
    presetUno,
    transformerDirectives,
    transformerVariantGroup,
} from 'unocss';
import presetAnimations from 'unocss-preset-animations';

export default defineConfig({
    presets: [presetAttributify(), presetUno(), presetAnimations() as never],
    transformers: [transformerDirectives(), transformerVariantGroup()],
    theme: {
        colors: {
            fs: {
                overlay: {
                    1: 'var(--fs-overlay-1)',
                    2: 'var(--fs-overlay-2)',
                    3: 'var(--fs-overlay-3)',
                    4: 'var(--fs-overlay-4)',
                },
                background: 'var(--fs-background)',
                accent: 'var(--fs-accent)',
            },
        },
    },
});
