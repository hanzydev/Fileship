import { existsSync, readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';

const file = fileURLToPath(new URL('./.data/docs/versions.json', import.meta.url));

export const docsVersions: {
    stable: string;
    experimental: string;
    extracted: '' | 'stable' | 'experimental';
} = existsSync(file)
    ? JSON.parse(readFileSync(file, 'utf8'))
    : { stable: '', experimental: '', extracted: '' };
