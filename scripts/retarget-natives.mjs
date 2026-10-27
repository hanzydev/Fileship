import {
    closeSync,
    cpSync,
    existsSync,
    lstatSync,
    openSync,
    readFileSync,
    readSync,
    readdirSync,
    rmSync,
} from 'node:fs';
import { basename, dirname, join, relative } from 'node:path';

const [root, platformArch] = process.argv.slice(2);
const arch = platformArch === 'amd64' ? 'x64' : platformArch;
const machines = { x64: 0x3e, arm64: 0xb7 };
const store = join(process.cwd(), 'node_modules/.pnpm');

if (!machines[arch]) throw new Error(`Unsupported architecture: ${platformArch}`);

const walk = (dir, visit) => {
    for (const entry of readdirSync(dir)) {
        const path = join(dir, entry);
        const stat = lstatSync(path);
        if (stat.isSymbolicLink()) continue;
        visit(path, stat);
        if (stat.isDirectory() && existsSync(path)) walk(path, visit);
    }
};

const readPackage = (dir) => {
    const file = join(dir, 'package.json');
    return existsSync(file) ? JSON.parse(readFileSync(file, 'utf8')) : null;
};

const findInStore = (name, version) => {
    const prefix = `${name.replace('/', '+')}@${version ?? ''}`;
    const matches = readdirSync(store).filter(
        (dir) => dir.startsWith(prefix) && (!version || /^(_|$)/.test(dir.slice(prefix.length))),
    );
    if (matches.length !== 1) {
        throw new Error(`Expected one ${prefix} in the pnpm store, found ${matches.length}`);
    }
    return join(store, matches[0], 'node_modules', name);
};

const copyPackage = (source, target) =>
    cpSync(source, target, {
        recursive: true,
        filter: (path) => path === source || basename(path) !== 'node_modules',
    });

const packages = [];
const prebuilds = [];
const napiDirs = [];

walk(root, (path, stat) => {
    if (!stat.isDirectory()) return;
    const pkg = readPackage(path);
    if (pkg?.name && pkg.cpu && !pkg.cpu.includes(arch)) packages.push({ path, pkg });
    if (basename(path) === 'prebuilds' && readPackage(dirname(path))?.name) prebuilds.push(path);
    if (/^napi-v\d+$/.test(basename(path))) napiDirs.push(path);
});

for (const { path, pkg } of packages) {
    const name = pkg.name.replace(/(?<=[-/])(x64|arm64)(?=-|$)/, arch);
    if (name === pkg.name) throw new Error(`Cannot find the ${arch} build of ${pkg.name}`);

    const target = join(dirname(path), name.split('/').pop());
    if (!existsSync(target)) copyPackage(findInStore(name), target);
    rmSync(path, { recursive: true });
    console.log(`${pkg.name} -> ${name}`);
}

for (const path of prebuilds) {
    const wanted = `linux-${arch}`;
    const entries = readdirSync(path);
    const pkg = readPackage(dirname(path));

    for (const entry of entries) {
        if (entry !== wanted) rmSync(join(path, entry), { recursive: true });
    }

    if (!entries.includes(wanted)) {
        const source = join(findInStore(pkg.name, pkg.version), 'prebuilds', wanted);
        if (existsSync(source)) {
            cpSync(source, join(path, wanted), { recursive: true });
            console.log(`${pkg.name} prebuild -> ${wanted}`);
        } else {
            console.log(`${pkg.name} has no ${wanted} prebuild`);
        }
    }
}

for (const path of napiDirs) {
    for (const os of readdirSync(path)) {
        if (!lstatSync(join(path, os)).isDirectory()) continue;
        for (const cpu of readdirSync(join(path, os))) {
            if (os !== 'linux' || cpu !== arch) rmSync(join(path, os, cpu), { recursive: true });
        }
    }
}

const header = Buffer.alloc(20);
const mismatched = [];

walk(root, (path, stat) => {
    if (!stat.isFile() || stat.size < header.length) return;
    const fd = openSync(path, 'r');
    readSync(fd, header, 0, header.length, 0);
    closeSync(fd);
    if (header.readUInt32BE(0) === 0x7f454c46 && header.readUInt16LE(18) !== machines[arch]) {
        mismatched.push(relative(root, path));
    }
});

if (mismatched.length) {
    throw new Error(`Native files that are not ${arch}:\n${mismatched.join('\n')}`);
}
