import { defineConfig } from 'tsdown';

const shared = {
    outDir: 'dist-electron',
    format: 'cjs',
    platform: 'node',
    target: 'node24',
    external: ['electron'],
    noExternal: [/.*/],
    clean: false,
    dts: false,
    deps: { onlyBundle: false },
} as const;

export default defineConfig([
    { ...shared, entry: { main: 'electron/main.ts' } },
    { ...shared, entry: { preload: 'electron/preload.ts' } },
]);
