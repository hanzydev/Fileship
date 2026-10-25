import { mkdirSync, readdirSync } from 'node:fs';
import { join } from 'node:path';

import { compileGroups, moveAsideLocked, objectFiles, root, runWithMsvc } from './msvc.mjs';

if (process.platform !== 'win32') {
    console.log('Skipping the overlay demo build, it needs Windows and MSVC.');
    process.exit(0);
}

const native = join(root, 'native');
const output = join(root, 'build', 'overlay-demo');
const objects = join(output, 'obj');
const executable = join(output, 'overlay-demo.exe');

const sources = (folder) =>
    readdirSync(join(native, folder))
        .filter((file) => file.endsWith('.cpp'))
        .map((file) => `"${join(native, folder, file)}"`);

const groups = {
    shared: sources('shared'),
    ui: sources('ui'),
    demo: sources(join('ui', 'demo')),
};

const flags = [
    '/std:c++20 /EHsc /O2 /MT /W4 /permissive- /utf-8 /MP',
    '/DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN',
    `/I"${native}" /I"${join(root, 'build', 'generated')}"`,
].join(' ');

mkdirSync(objects, { recursive: true });
moveAsideLocked(output, 'overlay-demo.exe');

runWithMsvc(join(output, 'build.cmd'), [
    ...compileGroups(objects, flags, groups),
    [
        `link /nologo /OUT:"${executable}"`,
        ...objectFiles(objects, groups),
        '/SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup',
        `/MANIFEST:EMBED /MANIFESTINPUT:"${join(native, 'host', 'host.manifest')}"`,
        'd2d1.lib d3d11.lib dxgi.lib dwrite.lib dcomp.lib windowscodecs.lib winhttp.lib ole32.lib',
        'oleaut32.lib windowsapp.lib user32.lib gdi32.lib dwmapi.lib imm32.lib advapi32.lib comdlg32.lib',
        'shell32.lib shcore.lib psapi.lib',
        '|| exit /b 1',
    ].join(' '),
]);
