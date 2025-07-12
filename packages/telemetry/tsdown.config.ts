import { defineConfig } from 'tsdown';

export default defineConfig({
    entry: ['./src'],
    target: 'esnext',
    dts: true,
    minify: true,
});
