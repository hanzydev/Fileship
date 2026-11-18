import { cpSync, mkdirSync } from 'node:fs';
import { join } from 'node:path';

import { native, root } from '../native.mjs';
import {
    compileGroups,
    moveAsideLocked,
    objectFiles,
    runWithMsvc,
    sourceFiles,
    writeVersionResource,
} from './msvc.mjs';

const source = join(native, 'harbor');
const output = join(root, 'build', 'harbor');
const objects = join(output, 'obj');
const executable = join(output, 'fileship-harbor.exe');
const resource = join(output, 'harbor.rc');
const compiledResource = join(output, 'harbor.res');

const groups = {
    harbor: sourceFiles(source),
    shared: [join(native, 'shared', 'http.cpp')],
    'shared-windows': [join(native, 'shared', 'windows', 'http.cpp')],
};
const flags = [
    '/std:c++20 /EHsc /O2 /MT /W4 /permissive- /utf-8 /MP',
    '/DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN',
    `/I"${native}"`,
].join(' ');

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
    ...compileGroups(objects, flags, groups),
    [
        `link /nologo /OUT:"${executable}"`,
        ...objectFiles(objects, groups),
        `"${compiledResource}"`,
        '/SUBSYSTEM:WINDOWS /ENTRY:wmainCRTStartup',
        'cldapi.lib windowsapp.lib winhttp.lib advapi32.lib ole32.lib shell32.lib crypt32.lib',
        'windowscodecs.lib shlwapi.lib gdi32.lib bcrypt.lib',
        '|| exit /b 1',
    ].join(' '),
]);
