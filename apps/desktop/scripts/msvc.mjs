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
import { basename, extname, join, resolve } from 'node:path';

export const root = resolve(import.meta.dirname, '..');

const vswhere = join(
    process.env['ProgramFiles(x86)'] ?? 'C:\\Program Files (x86)',
    'Microsoft Visual Studio',
    'Installer',
    'vswhere.exe',
);

const findInstallation = () => {
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
    return installation;
};

export const sourceFiles = (directory) =>
    readdirSync(directory)
        .filter((name) => name.endsWith('.cpp'))
        .map((name) => `"${join(directory, name)}"`);

export const compileGroups = (objects, flags, groups) =>
    Object.entries(groups)
        .filter(([, sources]) => sources.length)
        .map(([name, sources]) => {
            const folder = join(objects, name);
            rmSync(folder, { recursive: true, force: true });
            mkdirSync(folder, { recursive: true });
            return `cl /nologo /c ${flags} /Fo"${folder}\\\\" ${sources.join(' ')} || exit /b 1`;
        });

export const objectFiles = (objects, groups) =>
    Object.entries(groups)
        .filter(([, sources]) => sources.length)
        .map(([name]) => `"${join(objects, name)}\\*.obj"`);

export const moveAsideLocked = (output, file) => {
    const extension = extname(file);
    const base = basename(file, extension);

    for (const name of readdirSync(output).filter((item) => item.startsWith(`${base}.old-`))) {
        try {
            rmSync(join(output, name));
        } catch {
            continue;
        }
    }

    if (existsSync(join(output, file)))
        renameSync(join(output, file), join(output, `${base}.old-${Date.now()}${extension}`));
};

const quote = (value) => `"${value.replaceAll('"', '""')}"`;

export const writeVersionResource = (path, { description, name, fileName, type }) => {
    const { version, author } = JSON.parse(readFileSync(join(root, 'package.json'), 'utf8'));
    const [major = 0, minor = 0, patch = 0] = version.split(/[.-]/).map(Number);
    const numeric = [major, minor, patch, 0].join(',');
    const company = author.replace(/\s*<.*>$/, '');

    writeFileSync(
        path,
        [
            '#pragma code_page(65001)',
            '#include <winres.h>',
            `1 ICON ${quote(join(root, 'build', 'icon.ico').replaceAll('\\', '\\\\'))}`,
            'VS_VERSION_INFO VERSIONINFO',
            `FILEVERSION ${numeric}`,
            `PRODUCTVERSION ${numeric}`,
            'FILEFLAGSMASK 0x3fL',
            'FILEOS VOS_NT_WINDOWS32',
            `FILETYPE ${type}`,
            'BEGIN',
            '    BLOCK "StringFileInfo"',
            '    BEGIN',
            '        BLOCK "040904b0"',
            '        BEGIN',
            `            VALUE "CompanyName", ${quote(company)}`,
            `            VALUE "FileDescription", ${quote(description)}`,
            `            VALUE "FileVersion", ${quote(version)}`,
            `            VALUE "InternalName", ${quote(name)}`,
            `            VALUE "LegalCopyright", ${quote(`Copyright © 2024-present ${company} and Fileship contributors`)}`,
            `            VALUE "OriginalFilename", ${quote(fileName)}`,
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
};

export const runWithMsvc = (script, commands) => {
    const vcvars = join(findInstallation(), 'VC', 'Auxiliary', 'Build', 'vcvars64.bat');

    writeFileSync(
        script,
        ['@echo off', `call "${vcvars}" >nul || exit /b 1`, ...commands].join('\r\n'),
    );

    const result = spawnSync('cmd.exe', ['/d', '/c', script], { stdio: 'inherit' });
    if (result.status !== 0) process.exit(result.status ?? 1);
};
