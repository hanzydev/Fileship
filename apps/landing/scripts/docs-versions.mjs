import { execFileSync } from 'node:child_process';
import { mkdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

import semver from 'semver';

const root = fileURLToPath(new URL('../../../', import.meta.url));
const output = fileURLToPath(new URL('../.data/docs/', import.meta.url));
const redirects = fileURLToPath(new URL('../public/_redirects', import.meta.url));

const git = (...args) =>
    execFileSync('git', args, {
        cwd: root,
        encoding: 'buffer',
        maxBuffer: 1024 ** 3,
        stdio: ['ignore', 'pipe', 'ignore'],
    });

const findRef = (branch) =>
    [`refs/heads/${branch}`, `refs/remotes/origin/${branch}`].find((ref) => {
        try {
            git('rev-parse', '--verify', `${ref}^{commit}`);
            return true;
        } catch {
            return false;
        }
    });

const branch = (() => {
    try {
        return git('rev-parse', '--abbrev-ref', 'HEAD').toString().trim();
    } catch {
        return '';
    }
})();

const live = branch === 'next' ? 'experimental' : 'stable';
const otherRef = findRef(live === 'stable' ? 'next' : 'main');

const liveVersion = JSON.parse(readFileSync(`${root}package.json`, 'utf8')).version;
const otherVersion = otherRef && JSON.parse(git('show', `${otherRef}:package.json`)).version;

const [stable, candidate] =
    live === 'stable' ? [liveVersion, otherVersion] : [otherVersion ?? liveVersion, liveVersion];
const experimental = candidate && semver.gt(candidate, stable) ? candidate : '';

const extracted = live === 'stable' ? experimental && 'experimental' : otherRef && 'stable';
const versions = { stable, experimental, extracted: extracted || '' };

rmSync(output, { recursive: true, force: true });
mkdirSync(output, { recursive: true });

if (versions.extracted) {
    const version = versions[versions.extracted];
    const files = git('ls-tree', '-r', '--name-only', otherRef, '--', 'docs')
        .toString()
        .split('\n')
        .filter(Boolean);

    for (const file of files) {
        const target = `${output}${version}/${file.slice('docs/'.length)}`;
        mkdirSync(dirname(target), { recursive: true });
        writeFileSync(target, git('show', `${otherRef}:${file}`));
    }
}

if (experimental) rmSync(redirects, { force: true });
else
    writeFileSync(
        redirects,
        '/docs/experimental /docs 302\n/docs/experimental/* /docs/:splat 302\n',
    );

writeFileSync(`${output}versions.json`, `${JSON.stringify(versions, null, 4)}\n`);

console.log(
    `Docs: stable ${stable}${experimental ? `, experimental ${experimental}` : ''}${
        versions.extracted ? ` (${versions.extracted} from ${otherRef})` : ''
    }`,
);
