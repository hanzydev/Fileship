import {
    defineConfig,
    presetAttributify,
    presetUno,
    transformerDirectives,
    transformerVariantGroup,
} from 'unocss';
import presetAnimations from 'unocss-preset-animations';

export default defineConfig({
    presets: [presetAttributify(), presetUno(), presetAnimations()],
    transformers: [transformerDirectives(), transformerVariantGroup()],
    theme: {
        colors: {
            fs: {
                1: 'var(--color-fs-1)',
                2: 'var(--color-fs-2)',
                3: 'var(--color-fs-3)',
                4: 'var(--color-fs-4)',
                5: 'var(--color-fs-5)',
                accent: 'var(--color-fs-accent)',
            },
        },
        fontFamily: {
            sans: ['Montserrat', 'sans-serif'],
        },
    },
});
