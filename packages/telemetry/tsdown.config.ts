import { defineConfig } from 'tsdown';

export default defineConfig({
    target: 'esnext',
    dts: true,
    minify: true,
});
