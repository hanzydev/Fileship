import { mkdirSync, readdirSync } from 'node:fs';
import { dirname, join } from 'node:path';

import {
    compileGroups,
    moveAsideLocked,
    objectFiles,
    root,
    runWithMsvc,
    writeVersionResource,
} from './msvc.mjs';

if (process.platform !== 'win32') {
    console.log('Skipping the native host build, it needs Windows and MSVC.');
    process.exit(0);
}

const native = join(root, 'native');
const output = join(root, 'build', 'host');
const objects = join(output, 'obj');
const executable = join(output, 'fileship-host.exe');
const resource = join(output, 'host.rc');
const compiledResource = join(output, 'host.res');

const groupSources = (...folders) => {
    const groups = {};

    for (const folder of folders) {
        for (const file of readdirSync(join(native, folder), { recursive: true })) {
            if (!file.endsWith('.cpp') || join(folder, file).startsWith(join('ui', 'demo')))
                continue;

            const group = join(folder, dirname(file)).replaceAll('\\', '-').replace(/-\.$/, '');
            (groups[group] ??= []).push(`"${join(native, folder, file)}"`);
        }
    }

    return groups;
};

const groups = groupSources('shared', 'ui', 'host', join('core', 'lib'));
const flags = [
    '/std:c++20 /EHsc /O2 /MT /W4 /permissive- /utf-8 /MP',
    '/DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN',
    `/I"${native}" /I"${join(root, 'build', 'generated')}"`,
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
