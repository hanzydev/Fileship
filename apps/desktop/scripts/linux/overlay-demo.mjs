import { mkdirSync } from 'node:fs';
import { join } from 'node:path';

import { groupSources, root } from '../native.mjs';
import { compileGroups, compilerFlags, link, linkerFlags } from './gcc.mjs';

const PACKAGES = ['glib-2.0', 'gio-2.0', 'libsoup-3.0', 'gtk4', 'pangoft2', 'freetype2', 'fontconfig'];

const output = join(root, 'build', 'overlay-demo');

mkdirSync(output, { recursive: true });

const objects = await compileGroups(
    join(output, 'obj'),
    compilerFlags(PACKAGES),
    groupSources(['shared', 'ui']),
);
await link(join(output, 'overlay-demo'), objects, linkerFlags(PACKAGES));
