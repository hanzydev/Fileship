import { spawnSync } from 'node:child_process';
import { mkdirSync } from 'node:fs';
import { join } from 'node:path';

import { groupSources, native, root } from '../native.mjs';
import { compileGroups, moveAsideLocked, objectFiles, runWithMsvc } from './msvc.mjs';

const output = join(root, 'build', 'tests');
const objects = join(output, 'obj');
const executable = join(output, 'tests.exe');

const groups = groupSources(['shared', 'tests']);
groups.recording = [
    join(native, 'host', 'recording', 'command.cpp'),
    join(native, 'host', 'windows', 'recording', 'videoInput.cpp'),
];

const flags = [
    '/std:c++20 /EHsc /O2 /MT /W4 /permissive- /utf-8 /MP',
    '/DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN',
    `/I"${native}"`,
].join(' ');

mkdirSync(objects, { recursive: true });
moveAsideLocked(output, 'tests.exe');

runWithMsvc(join(output, 'build.cmd'), [
    ...compileGroups(objects, flags, groups),
    [
        `link /nologo /OUT:"${executable}"`,
        ...objectFiles(objects, groups),
        '/SUBSYSTEM:CONSOLE winhttp.lib bcrypt.lib windowsapp.lib',
        '|| exit /b 1',
    ].join(' '),
]);

const result = spawnSync(executable, process.argv.slice(3), { stdio: 'inherit' });
process.exit(result.status ?? 1);
