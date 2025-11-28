import {
    defineConfig,
    presetAttributify,
    presetWind3,
    transformerDirectives,
    transformerVariantGroup,
} from 'unocss';
import presetAnimations from 'unocss-preset-animations';
import { presetScrollbar } from 'unocss-preset-scrollbar';

export default defineConfig({
    presets: [presetAttributify(), presetWind3(), presetAnimations(), presetScrollbar()],
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
                muted: {
                    1: 'var(--fs-muted-1)',
                    2: 'var(--fs-muted-2)',
                    3: 'var(--fs-muted-3)',
                },
                background: 'var(--fs-background)',
                accent: 'var(--fs-accent)',
            },
        },
    },
});
