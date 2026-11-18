import { mkdirSync } from 'node:fs';
import { join } from 'node:path';

import { groupSources, root } from '../native.mjs';
import { compileGroups, compilerFlags, link, linkerFlags } from './gcc.mjs';

const PACKAGES = ['glib-2.0', 'gio-2.0', 'libsoup-3.0'];

const output = join(root, 'build', 'harbor');

mkdirSync(output, { recursive: true });

const objects = await compileGroups(
    join(output, 'obj'),
    compilerFlags(PACKAGES),
    groupSources(['shared', 'harbor']),
);
await link(join(output, 'fileship-harbor'), objects, [
    ...linkerFlags(PACKAGES),
    '-Wl,-Bstatic',
    '-lfuse3',
    '-Wl,-Bdynamic',
    '-ldl',
]);
