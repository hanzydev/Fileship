import { mkdirSync, readFileSync } from 'node:fs';
import { join } from 'node:path';

import { groupSources, root } from '../native.mjs';
import { compileGroups, compilerFlags, link, linkerFlags } from './gcc.mjs';

const PACKAGES = [
    'glib-2.0',
    'gio-2.0',
    'gio-unix-2.0',
    'libsoup-3.0',
    'libsecret-1',
    'sqlite3',
    'gdk-pixbuf-2.0',
];

const { version } = JSON.parse(readFileSync(join(root, 'package.json'), 'utf8'));
const output = join(root, 'build', 'host');

mkdirSync(output, { recursive: true });

const objects = await compileGroups(
    join(output, 'obj'),
    [...compilerFlags(PACKAGES), `-DFILESHIP_VERSION="${version}"`],
    groupSources(['shared', 'host', 'core']),
);
await link(join(output, 'fileship-host'), objects, linkerFlags(PACKAGES));
