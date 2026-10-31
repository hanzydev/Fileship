import { execFileSync } from 'node:child_process';
import { mkdirSync, readdirSync, readFileSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';

import { requirePackages } from './gcc.mjs';

const RENAMED_PROTOCOLS = {
    'wlr-data-control-unstable-v1': {
        source: 'ext-data-control-v1',
        from: /ext_data_control_/g,
        to: 'zwlr_data_control_',
    },
};

export const generateProtocols = (protocols, output) => {
    requirePackages(['wayland-scanner']);

    const scanner = execFileSync('pkg-config', ['--variable=wayland_scanner', 'wayland-scanner'], {
        encoding: 'utf8',
    }).trim();
    const sources = join(output, 'xml');
    mkdirSync(sources, { recursive: true });

    for (const file of readdirSync(protocols).filter((name) => name.endsWith('.xml'))) {
        writeFileSync(join(sources, file), readFileSync(join(protocols, file)));
    }

    for (const [name, { source, from, to }] of Object.entries(RENAMED_PROTOCOLS)) {
        const xml = readFileSync(join(protocols, `${source}.xml`), 'utf8');
        writeFileSync(join(sources, `${name}.xml`), xml.replace(from, to));
    }

    for (const file of readdirSync(sources).filter((name) => name.endsWith('.xml'))) {
        const name = file.replace(/\.xml$/, '');
        execFileSync(scanner, [
            'client-header',
            join(sources, file),
            join(output, `${name}-client.h`),
        ]);
        execFileSync(scanner, [
            'private-code',
            join(sources, file),
            join(output, `${name}-protocol.c`),
        ]);
    }
};
