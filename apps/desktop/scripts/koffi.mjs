import { copyFileSync, mkdirSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname, join } from 'node:path';

const require = createRequire(import.meta.url);
const koffi = dirname(require.resolve('koffi'));
const native = createRequire(join(koffi, 'index.cjs')).resolve(
    '@koromix/koffi-win32-x64/package.json',
);
const target = 'build/koffi/win32_x64';

mkdirSync(target, { recursive: true });
copyFileSync(join(dirname(native), 'win32_x64', 'koffi.node'), join(target, 'koffi.node'));
