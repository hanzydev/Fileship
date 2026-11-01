import { execFileSync } from 'node:child_process';
import {
    copyFileSync,
    cpSync,
    existsSync,
    mkdirSync,
    readdirSync,
    realpathSync,
    rmSync,
} from 'node:fs';
import { join } from 'node:path';

import { buildExecutable } from './gcc.mjs';
import {
    compileGroups,
    moveAsideLocked,
    objectFiles,
    root,
    runWithMsvc,
    sourceFiles,
    writeVersionResource,
} from './msvc.mjs';

const native = join(root, 'native');
const shared = join(native, 'shared');
const output = join(root, 'build', 'harbor');
const objects = join(output, 'obj');

if (process.platform === 'linux') {
    const source = join(native, 'harbor', 'linux');
    const executable = join(output, 'fileship-harbor');

    mkdirSync(objects, { recursive: true });
    await buildExecutable({
        objects,
        executable,
        groups: {
            shared: ['http.cpp', 'json.cpp'].map((file) => join(shared, file)),
            'shared-linux': ['http.cpp', 'system.cpp'].map((file) => join(shared, 'linux', file)),
            harbor: readdirSync(source)
                .filter((file) => file.endsWith('.cpp'))
                .map((file) => join(source, file)),
        },
        packages: ['fuse3', 'libcurl', 'glib-2.0'],
        flags: [`-I${native}`],
        libraries: ['-Wl,-rpath,$ORIGIN'],
    });

    for (const file of readdirSync(output)) {
        if (file.startsWith('libfuse3.so')) rmSync(join(output, file));
    }

    const library = execFileSync('ldd', [executable], { encoding: 'utf8' }).match(
        /^\s*(libfuse3\.so\.\d+) => (\S+)/m,
    );
    copyFileSync(realpathSync(library[2]), join(output, library[1]));

    const license = '/usr/share/common-licenses/LGPL-2.1';
    if (existsSync(license)) copyFileSync(license, join(output, 'LICENSE.libfuse.txt'));
    process.exit(0);
}

if (process.platform !== 'win32') {
    console.log(`Skipping the Harbor helper build, ${process.platform} is not supported.`);
    process.exit(0);
}

const source = join(native, 'harbor', 'windows');
const executable = join(output, 'fileship-harbor.exe');
const resource = join(output, 'harbor.rc');
const compiledResource = join(output, 'harbor.res');
const groups = {
    shared: [`"${join(shared, 'http.cpp')}"`],
    'shared-windows': ['http.cpp', 'system.cpp'].map(
        (file) => `"${join(shared, 'windows', file)}"`,
    ),
};
const flags = [
    '/std:c++20 /EHsc /O2 /MT /W4 /permissive- /utf-8',
    '/DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN',
    `/I"${native}"`,
].join(' ');

mkdirSync(objects, { recursive: true });
cpSync(join(native, 'harbor', 'icons'), join(output, 'icons'), { recursive: true });
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
        `cl /nologo ${flags}`,
        `/Fo"${objects}\\\\" /Fe"${executable}"`,
        ...sourceFiles(source),
        ...objectFiles(objects, groups),
        `"${compiledResource}"`,
        '/link /SUBSYSTEM:WINDOWS /ENTRY:wmainCRTStartup',
        'cldapi.lib windowsapp.lib winhttp.lib advapi32.lib ole32.lib shell32.lib crypt32.lib',
        'windowscodecs.lib shlwapi.lib gdi32.lib bcrypt.lib',
    ].join(' '),
]);
