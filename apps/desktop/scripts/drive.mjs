import { execFileSync, spawnSync } from 'node:child_process';
import {
    existsSync,
    mkdirSync,
    readdirSync,
    readFileSync,
    renameSync,
    rmSync,
    writeFileSync,
} from 'node:fs';
import { join, resolve } from 'node:path';

const root = resolve(import.meta.dirname, '..');
const source = join(root, 'native', 'drive');
const output = join(root, 'build', 'drive');
const objects = join(output, 'obj');

const vswhere = join(
    process.env['ProgramFiles(x86)'] ?? 'C:\\Program Files (x86)',
    'Microsoft Visual Studio',
    'Installer',
    'vswhere.exe',
);

if (!existsSync(vswhere)) throw new Error('Visual Studio Build Tools with C++ are required.');

const installation = execFileSync(
    vswhere,
    [
        '-latest',
        '-products',
        '*',
        '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
        '-property',
        'installationPath',
    ],
    { encoding: 'utf8' },
).trim();

if (!installation) throw new Error('The MSVC x64 toolset is not installed.');

const files = readdirSync(source)
    .filter((name) => name.endsWith('.cpp'))
    .map((name) => `"${join(source, name)}"`);

mkdirSync(objects, { recursive: true });

const executable = join(output, 'fileship-drive.exe');

for (const name of readdirSync(output).filter((item) => item.startsWith('fileship-drive.old-'))) {
    try {
        rmSync(join(output, name));
    } catch {
        continue;
    }
}

if (existsSync(executable))
    renameSync(executable, join(output, `fileship-drive.old-${Date.now()}.exe`));

const { version, author } = JSON.parse(readFileSync(join(root, 'package.json'), 'utf8'));
const [major = 0, minor = 0, patch = 0] = version.split(/[.-]/).map(Number);
const numeric = [major, minor, patch, 0].join(',');
const company = author.replace(/\s*<.*>$/, '');
const resource = join(output, 'drive.rc');
const compiledResource = join(output, 'drive.res');

const quote = (value) => `"${value.replaceAll('"', '""')}"`;

writeFileSync(
    resource,
    [
        '#pragma code_page(65001)',
        '#include <winres.h>',
        `1 ICON ${quote(join(root, 'build', 'icon.ico').replaceAll('\\', '\\\\'))}`,
        'VS_VERSION_INFO VERSIONINFO',
        `FILEVERSION ${numeric}`,
        `PRODUCTVERSION ${numeric}`,
        'FILEFLAGSMASK 0x3fL',
        'FILEOS VOS_NT_WINDOWS32',
        'FILETYPE VFT_APP',
        'BEGIN',
        '    BLOCK "StringFileInfo"',
        '    BEGIN',
        '        BLOCK "040904b0"',
        '        BEGIN',
        `            VALUE "CompanyName", ${quote(company)}`,
        '            VALUE "FileDescription", "Fileship drive"',
        `            VALUE "FileVersion", ${quote(version)}`,
        '            VALUE "InternalName", "fileship-drive"',
        `            VALUE "LegalCopyright", ${quote(`Copyright © 2024-present ${company} and Fileship contributors`)}`,
        '            VALUE "OriginalFilename", "fileship-drive.exe"',
        '            VALUE "ProductName", "Fileship"',
        `            VALUE "ProductVersion", ${quote(version)}`,
        '        END',
        '    END',
        '    BLOCK "VarFileInfo"',
        '    BEGIN',
        '        VALUE "Translation", 0x409, 1200',
        '    END',
        'END',
        '',
    ].join('\r\n'),
);

const script = join(output, 'build.cmd');

writeFileSync(
    script,
    [
        '@echo off',
        `call "${join(installation, 'VC', 'Auxiliary', 'Build', 'vcvars64.bat')}" >nul || exit /b 1`,
        `rc /nologo /c65001 /fo "${compiledResource}" "${resource}" || exit /b 1`,
        [
            'cl /nologo /std:c++20 /EHsc /O2 /MT /W4 /permissive- /utf-8',
            '/DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN',
            `/Fo"${objects}\\\\" /Fe"${executable}"`,
            ...files,
            `"${compiledResource}"`,
            '/link cldapi.lib windowsapp.lib winhttp.lib advapi32.lib ole32.lib shell32.lib',
        ].join(' '),
    ].join('\r\n'),
);

const result = spawnSync('cmd.exe', ['/d', '/c', script], { stdio: 'inherit' });
if (result.status !== 0) process.exit(result.status ?? 1);
