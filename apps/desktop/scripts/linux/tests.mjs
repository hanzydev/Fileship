import { spawnSync } from 'node:child_process';
import { mkdirSync } from 'node:fs';
import { join } from 'node:path';

import { groupSources, native, root } from '../native.mjs';
import { compileGroups, compilerFlags, link, linkerFlags } from './gcc.mjs';

const PACKAGES = ['glib-2.0', 'gio-2.0', 'libsoup-3.0'];

const output = join(root, 'build', 'tests');
const executable = join(output, 'tests');

mkdirSync(output, { recursive: true });

const objects = await compileGroups(
    join(output, 'obj'),
    compilerFlags(PACKAGES),
    {
        ...groupSources(['shared', 'tests']),
        recording: [join(native, 'host', 'recording', 'command.cpp')],
    },
);
await link(executable, objects, linkerFlags(PACKAGES));

const result = spawnSync(executable, process.argv.slice(3), { stdio: 'inherit' });
process.exit(result.status ?? 1);
