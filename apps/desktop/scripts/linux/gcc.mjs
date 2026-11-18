import { execFileSync, spawn } from 'node:child_process';
import { mkdirSync, rmSync } from 'node:fs';
import { availableParallelism } from 'node:os';
import { basename, join } from 'node:path';

import { generated, native } from '../native.mjs';

const split = (text) => text.trim().split(/\s+/).filter(Boolean);

const run = (command, args) =>
    new Promise((resolve) => {
        spawn(command, args, { stdio: 'inherit' }).on('exit', (code) => {
            if (code !== 0) process.exit(code ?? 1);
            resolve();
        });
    });

export const compilerFlags = (packages) => [
    '-std=c++20',
    '-O2',
    '-Wall',
    '-Wextra',
    '-pthread',
    `-I${native}`,
    `-I${generated}`,
    ...split(execFileSync('pkg-config', ['--cflags', ...packages], { encoding: 'utf8' })),
];

export const linkerFlags = (packages) => [
    '-pthread',
    '-static-libstdc++',
    '-static-libgcc',
    ...split(execFileSync('pkg-config', ['--libs', ...packages], { encoding: 'utf8' })),
];

export const compileGroups = async (objects, flags, groups) => {
    const jobs = [];

    for (const [name, sources] of Object.entries(groups)) {
        const folder = join(objects, name);
        rmSync(folder, { recursive: true, force: true });
        mkdirSync(folder, { recursive: true });

        for (const source of sources) {
            jobs.push({ source, object: join(folder, `${basename(source, '.cpp')}.o`) });
        }
    }

    const queue = [...jobs];
    const work = async () => {
        for (let job = queue.shift(); job; job = queue.shift()) {
            await run('g++', ['-c', ...flags, '-o', job.object, job.source]);
        }
    };

    await Promise.all(Array.from({ length: availableParallelism() }, work));
    return jobs.map((job) => job.object);
};

export const link = (executable, objects, flags) =>
    run('g++', ['-o', executable, ...objects, ...flags]);
