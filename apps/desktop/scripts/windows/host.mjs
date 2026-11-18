import { mkdirSync } from 'node:fs';
import { join } from 'node:path';

import { generated, groupSources, native, root } from '../native.mjs';
import {
    compileGroups,
    moveAsideLocked,
    objectFiles,
    runWithMsvc,
    writeVersionResource,
} from './msvc.mjs';

const output = join(root, 'build', 'host');
const objects = join(output, 'obj');
const executable = join(output, 'fileship-host.exe');
const resource = join(output, 'host.rc');
const compiledResource = join(output, 'host.res');

const groups = groupSources(
    ['shared', 'ui', 'host', 'core'],
    (path) => !path.startsWith(join('ui', 'demo')),
);
const flags = [
    '/std:c++20 /EHsc /O2 /MT /W4 /permissive- /utf-8 /MP',
    '/DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN',
    `/I"${native}" /I"${generated}"`,
].join(' ');

mkdirSync(objects, { recursive: true });
moveAsideLocked(output, 'fileship-host.exe');

writeVersionResource(resource, {
    description: 'Fileship',
    name: 'fileship-host',
    fileName: 'fileship-host.exe',
    type: 'VFT_APP',
});

runWithMsvc(join(output, 'build.cmd'), [
    `rc /nologo /c65001 /fo "${compiledResource}" "${resource}" || exit /b 1`,
    ...compileGroups(objects, flags, groups),
    [
        `link /nologo /OUT:"${executable}"`,
        ...objectFiles(objects, groups),
        `"${compiledResource}"`,
        '/SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup',
        `/MANIFEST:EMBED /MANIFESTINPUT:"${join(native, 'host', 'host.manifest')}"`,
        'd2d1.lib d3d11.lib dxgi.lib dwrite.lib dcomp.lib windowscodecs.lib winhttp.lib winsqlite3.lib',
        'crypt32.lib bcrypt.lib shell32.lib ole32.lib oleaut32.lib windowsapp.lib mmdevapi.lib',
        'avrt.lib winmm.lib user32.lib gdi32.lib dwmapi.lib imm32.lib advapi32.lib comctl32.lib',
        'comdlg32.lib shlwapi.lib',
        '|| exit /b 1',
    ].join(' '),
]);
