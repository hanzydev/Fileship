import { mkdirSync, readdirSync } from 'node:fs';
import { join } from 'node:path';

import { buildExecutable } from './gcc.mjs';
import { compileGroups, moveAsideLocked, objectFiles, root, runWithMsvc } from './msvc.mjs';

const PLATFORM_FOLDERS = { win32: 'windows', linux: 'linux' };

const platform = PLATFORM_FOLDERS[process.platform];
if (!platform) {
    console.log(`Skipping the overlay demo build, ${process.platform} is not supported.`);
    process.exit(0);
}

const native = join(root, 'native');
const generated = join(root, 'build', 'generated');
const output = join(root, 'build', 'overlay-demo');
const objects = join(output, 'obj');

const sources = (folder) =>
    readdirSync(join(native, folder))
        .filter((file) => file.endsWith('.cpp'))
        .map((file) => join(native, folder, file));

const groups = {
    shared: sources('shared'),
    [`shared-${platform}`]: sources(join('shared', platform)),
    ui: sources('ui'),
    [`ui-${platform}`]: sources(join('ui', platform)),
    demo: sources(join('ui', 'demo')),
    [`demo-${platform}`]: sources(join('ui', 'demo', platform)),
};

mkdirSync(objects, { recursive: true });

if (platform === 'linux') {
    await buildExecutable({
        objects,
        executable: join(output, 'overlay-demo'),
        groups,
        packages: ['gtk4', 'libcurl', 'fontconfig'],
        flags: [`-I${native}`, `-I${generated}`],
    });
} else {
    const executable = join(output, 'overlay-demo.exe');
    const quoted = Object.fromEntries(
        Object.entries(groups).map(([name, files]) => [name, files.map((file) => `"${file}"`)]),
    );
    const flags = [
        '/std:c++20 /EHsc /O2 /MT /W4 /permissive- /utf-8 /MP',
        '/DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN',
        `/I"${native}" /I"${generated}"`,
    ].join(' ');

    moveAsideLocked(output, 'overlay-demo.exe');

    runWithMsvc(join(output, 'build.cmd'), [
        ...compileGroups(objects, flags, quoted),
        [
            `link /nologo /OUT:"${executable}"`,
            ...objectFiles(objects, quoted),
            '/SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup',
            `/MANIFEST:EMBED /MANIFESTINPUT:"${join(native, 'host', 'host.manifest')}"`,
            'd2d1.lib d3d11.lib dxgi.lib dwrite.lib dcomp.lib windowscodecs.lib winhttp.lib ole32.lib',
            'oleaut32.lib windowsapp.lib user32.lib gdi32.lib dwmapi.lib imm32.lib advapi32.lib comdlg32.lib',
            'shell32.lib shcore.lib psapi.lib',
            '|| exit /b 1',
        ].join(' '),
    ]);
}
