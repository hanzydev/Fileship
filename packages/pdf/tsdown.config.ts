import { defineConfig } from 'tsdown';

export default defineConfig({
    entry: { index: 'src/index.ts', 'pdf-worker': 'src/worker.ts' },
    target: 'esnext',
    dts: true,
});
