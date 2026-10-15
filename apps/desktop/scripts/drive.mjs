import { mkdirSync } from 'node:fs';
import { join } from 'node:path';

import { moveAsideLocked, root, runWithMsvc, sourceFiles, writeVersionResource } from './msvc.mjs';

const source = join(root, 'native', 'drive');
const output = join(root, 'build', 'drive');
const objects = join(output, 'obj');
const executable = join(output, 'fileship-drive.exe');
const resource = join(output, 'drive.rc');
const compiledResource = join(output, 'drive.res');

mkdirSync(objects, { recursive: true });
moveAsideLocked(output, 'fileship-drive.exe');

writeVersionResource(resource, {
    description: 'Fileship drive',
    name: 'fileship-drive',
    fileName: 'fileship-drive.exe',
    type: 'VFT_APP',
});

runWithMsvc(join(output, 'build.cmd'), [
    `rc /nologo /c65001 /fo "${compiledResource}" "${resource}" || exit /b 1`,
    [
        'cl /nologo /std:c++20 /EHsc /O2 /MT /W4 /permissive- /utf-8',
        '/DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN',
        `/Fo"${objects}\\\\" /Fe"${executable}"`,
        ...sourceFiles(source),
        `"${compiledResource}"`,
        '/link cldapi.lib windowsapp.lib winhttp.lib advapi32.lib ole32.lib shell32.lib',
    ].join(' '),
]);
