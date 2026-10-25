import { mkdirSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname, join } from 'node:path';

import { moveAsideLocked, root, runWithMsvc, sourceFiles, writeVersionResource } from './msvc.mjs';

if (process.platform !== 'win32') {
    console.log('Skipping the native addon build, it needs Windows and MSVC.');
    process.exit(0);
}

const require = createRequire(import.meta.url);
const headers = dirname(require.resolve('node-api-headers/package.json'));
const addonApi = dirname(require.resolve('node-addon-api/package.json'));

const source = join(root, 'native', 'core');
const output = join(root, 'build', 'native');
const objects = join(output, 'obj');
const addon = join(output, 'fileship-native.node');
const importLibrary = join(objects, 'node.lib');
const resource = join(output, 'native.rc');
const compiledResource = join(output, 'native.res');

mkdirSync(objects, { recursive: true });
moveAsideLocked(output, 'fileship-native.node');

writeVersionResource(resource, {
    description: 'Fileship native',
    name: 'fileship-native',
    fileName: 'fileship-native.node',
    type: 'VFT_DLL',
});

runWithMsvc(join(output, 'build.cmd'), [
    `lib /nologo /machine:x64 /def:"${join(headers, 'def', 'node_api.def')}" /out:"${importLibrary}" >nul || exit /b 1`,
    `rc /nologo /c65001 /fo "${compiledResource}" "${resource}" || exit /b 1`,
    [
        'cl /nologo /LD /std:c++20 /EHsc /O2 /MT /W4 /permissive- /utf-8',
        '/DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN',
        '/DNAPI_VERSION=8 /DNAPI_CPP_EXCEPTIONS /DNODE_ADDON_API_DISABLE_DEPRECATED',
        `/I"${join(headers, 'include')}" /I"${addonApi}"`,
        `/Fo"${objects}\\\\" /Fe"${addon}"`,
        ...sourceFiles(source),
        ...sourceFiles(join(source, 'lib')),
        `"${compiledResource}"`,
        `/link /IMPLIB:"${join(objects, 'fileship-native.lib')}" /DELAYLOAD:node.exe "${importLibrary}" delayimp.lib`,
        'user32.lib gdi32.lib dwmapi.lib dxgi.lib windowscodecs.lib ole32.lib oleaut32.lib winmm.lib',
    ].join(' '),
]);
