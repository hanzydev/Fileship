import { mkdirSync, readdirSync } from 'node:fs';
import { dirname, join, sep } from 'node:path';

import { buildExecutable } from './gcc.mjs';
import {
    compileGroups,
    moveAsideLocked,
    objectFiles,
    root,
    runWithMsvc,
    writeVersionResource,
} from './msvc.mjs';

const PLATFORM_FOLDERS = { win32: 'windows', linux: 'linux' };
const PENDING_ON_LINUX = [join('host', 'recording'), join('host', 'harbor')];

const platform = PLATFORM_FOLDERS[process.platform];
if (!platform) {
    console.log(`Skipping the native host build, ${process.platform} is not supported.`);
    process.exit(0);
}

const native = join(root, 'native');
const output = join(root, 'build', 'host');
const objects = join(output, 'obj');
const generated = join(root, 'build', 'generated');

const groupSources = (...folders) => {
    const groups = {};
    const others = Object.values(PLATFORM_FOLDERS).filter((folder) => folder !== platform);

    for (const folder of folders) {
        for (const file of readdirSync(join(native, folder), { recursive: true })) {
            const path = join(folder, file);
            if (
                !file.endsWith('.cpp') ||
                path.startsWith(join('ui', 'demo')) ||
                file.split(sep).some((part) => others.includes(part)) ||
                (platform === 'linux' &&
                    PENDING_ON_LINUX.some((pending) => path.startsWith(pending)))
            )
                continue;

            const group = join(folder, dirname(file)).replaceAll(sep, '-').replace(/-\.$/, '');
            (groups[group] ??= []).push(join(native, folder, file));
        }
    }

    return groups;
};

const groups = groupSources('shared', 'ui', 'host', 'core');
mkdirSync(objects, { recursive: true });

if (platform === 'linux') {
    await buildExecutable({
        objects,
        executable: join(output, 'fileship-host'),
        groups,
        packages: [
            'gtk4',
            'gio-unix-2.0',
            'libcurl',
            'sqlite3',
            'libsecret-1',
            'libpulse',
            'libpulse-simple',
            'x11',
            'xext',
            'xfixes',
            'xrandr',
            'fontconfig',
        ],
        flags: [`-I${native}`, `-I${generated}`],
    });
} else {
    const quoted = Object.fromEntries(
        Object.entries(groups).map(([name, sources]) => [
            name,
            sources.map((source) => `"${source}"`),
        ]),
    );
    const executable = join(output, 'fileship-host.exe');
    const resource = join(output, 'host.rc');
    const compiledResource = join(output, 'host.res');
    const flags = [
        '/std:c++20 /EHsc /O2 /MT /W4 /permissive- /utf-8 /MP',
        '/DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN',
        `/I"${native}" /I"${generated}"`,
    ].join(' ');

    moveAsideLocked(output, 'fileship-host.exe');

    writeVersionResource(resource, {
        description: 'Fileship',
        name: 'fileship-host',
        fileName: 'fileship-host.exe',
        type: 'VFT_APP',
    });

    runWithMsvc(join(output, 'build.cmd'), [
        `rc /nologo /c65001 /fo "${compiledResource}" "${resource}" || exit /b 1`,
        ...compileGroups(objects, flags, quoted),
        [
            `link /nologo /OUT:"${executable}"`,
            ...objectFiles(objects, quoted),
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
}
