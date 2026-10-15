import { defineConfig } from 'tsdown';

const isWatching = process.argv.includes('--watch');

const shared = {
    outDir: 'dist-electron',
    format: 'cjs',
    platform: 'node',
    target: 'node24',
    external: isWatching ? ['electron', 'koffi'] : ['electron'],
    noExternal: [/.*/],
    clean: false,
    dts: false,
    deps: { onlyBundle: false },
} as const;

export default defineConfig([
    { ...shared, entry: { main: 'electron/main.ts' } },
    { ...shared, entry: { preload: 'electron/preload.ts' } },
]);
