import { cpSync, mkdirSync } from 'node:fs';
import { join } from 'node:path';

import { moveAsideLocked, root, runWithMsvc, sourceFiles, writeVersionResource } from './msvc.mjs';

if (process.platform !== 'win32') {
    console.log('Skipping the Harbor helper build, it needs Windows and MSVC.');
    process.exit(0);
}

const source = join(root, 'native', 'harbor');
const output = join(root, 'build', 'harbor');
const objects = join(output, 'obj');
const executable = join(output, 'fileship-harbor.exe');
const resource = join(output, 'harbor.rc');
const compiledResource = join(output, 'harbor.res');

mkdirSync(objects, { recursive: true });
cpSync(join(source, 'icons'), join(output, 'icons'), { recursive: true });
moveAsideLocked(output, 'fileship-harbor.exe');

writeVersionResource(resource, {
    description: 'Fileship Harbor',
    name: 'fileship-harbor',
    fileName: 'fileship-harbor.exe',
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
        '/link /SUBSYSTEM:WINDOWS /ENTRY:wmainCRTStartup',
        'cldapi.lib windowsapp.lib winhttp.lib advapi32.lib ole32.lib shell32.lib crypt32.lib',
        'windowscodecs.lib shlwapi.lib gdi32.lib',
    ].join(' '),
]);
