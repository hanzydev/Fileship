import { execFileSync, spawn } from 'node:child_process';
import { mkdirSync, rmSync } from 'node:fs';
import { availableParallelism } from 'node:os';
import { basename, join } from 'node:path';

const PACKAGE_NAMES = {
    gtk4: 'libgtk-4-dev',
    libcurl: 'libcurl4-openssl-dev',
    sqlite3: 'libsqlite3-dev',
    'libsecret-1': 'libsecret-1-dev',
    libpulse: 'libpulse-dev',
    'libpulse-simple': 'libpulse-dev',
    'libpipewire-0.3': 'libpipewire-0.3-dev',
    fuse3: 'libfuse3-dev',
    x11: 'libx11-dev',
    xext: 'libxext-dev',
    xfixes: 'libxfixes-dev',
    xrandr: 'libxrandr-dev',
    fontconfig: 'libfontconfig-dev',
    'wayland-client': 'libwayland-dev',
    'wayland-scanner': 'libwayland-bin',
    'glib-2.0': 'libglib2.0-dev',
    'gio-2.0': 'libglib2.0-dev',
    'gio-unix-2.0': 'libglib2.0-dev',
};

const compiler = process.env.CXX ?? 'c++';

const pkgConfig = (flag, packages) =>
    execFileSync('pkg-config', [flag, ...packages], { encoding: 'utf8' })
        .trim()
        .split(/\s+/)
        .filter(Boolean);

export const requirePackages = (packages) => {
    const missing = packages.filter((name) => {
        try {
            execFileSync('pkg-config', ['--exists', name]);
            return false;
        } catch {
            return true;
        }
    });

    if (!missing.length) return;

    const install = [...new Set(missing.map((name) => PACKAGE_NAMES[name] ?? name))].join(' ');
    console.error(`Missing development packages for: ${missing.join(', ')}`);
    console.error(`On Debian or Ubuntu: sudo apt install ${install}`);
    process.exit(1);
};

const run = (command, args) =>
    new Promise((resolve, reject) => {
        const child = spawn(command, args, { stdio: 'inherit' });
        child.on('error', reject);
        child.on('exit', (code) =>
            code === 0 ? resolve() : reject(new Error(`${command} exited with ${code}`)),
        );
    });

export const buildExecutable = async ({
    objects,
    executable,
    groups,
    packages,
    flags = [],
    libraries = [],
}) => {
    requirePackages(packages);

    const compileFlags = [
        '-std=c++20',
        '-O2',
        '-Wall',
        '-Wextra',
        '-finput-charset=UTF-8',
        '-fvisibility=hidden',
        ...flags,
        ...pkgConfig('--cflags', packages),
    ];

    const jobs = [];
    for (const [name, sources] of Object.entries(groups)) {
        const folder = join(objects, name);
        rmSync(folder, { recursive: true, force: true });
        mkdirSync(folder, { recursive: true });

        for (const source of sources) {
            jobs.push({ source, object: join(folder, basename(source).replace(/\.cpp$/, '.o')) });
        }
    }

    let failed = false;
    const queue = [...jobs];
    const worker = async () => {
        while (queue.length) {
            const job = queue.shift();
            try {
                await run(compiler, [...compileFlags, '-c', job.source, '-o', job.object]);
            } catch {
                failed = true;
            }
        }
    };

    await Promise.all(Array.from({ length: availableParallelism() }, worker));
    if (failed) process.exit(1);

    await run(compiler, [
        '-o',
        executable,
        ...jobs.map((job) => job.object),
        ...pkgConfig('--libs', packages),
        ...libraries,
        '-pthread',
        '-ldl',
    ]);
};
