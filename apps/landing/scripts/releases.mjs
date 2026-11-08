import { execFileSync } from 'node:child_process';
import { mkdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';

const root = fileURLToPath(new URL('../../../', import.meta.url));
const output = fileURLToPath(new URL('../.data/releases/', import.meta.url));

const changelog = readFileSync(`${root}CHANGELOG.md`, 'utf8').replace(/\r\n/g, '\n');

const tagDates = () => {
    try {
        const lines = execFileSync(
            'git',
            ['for-each-ref', 'refs/tags', '--format=%(refname:short) %(creatordate:short)'],
            { cwd: root, encoding: 'utf8' },
        );

        return new Map(
            lines
                .trim()
                .split('\n')
                .map((line) => line.split(' ')),
        );
    } catch {
        return new Map();
    }
};

const dates = tagDates();

const sections = changelog.split(/^## (?=v\d)/m).slice(1);

rmSync(output, { recursive: true, force: true });
mkdirSync(output, { recursive: true });

sections.forEach((section, index) => {
    const [heading, ...rest] = section.split('\n');
    const version = heading.trim();
    let body = rest.join('\n').trim();

    const compare = body.match(/^\[compare changes\]\((.+?)\)\s*/);
    if (compare) body = body.slice(compare[0].length).trim();

    const summary = body.startsWith('#')
        ? ''
        : body
              .split(/\n\s*\n/)[0]
              .replace(/\s+/g, ' ')
              .trim();

    const desktop = body.match(/Fileship for Windows (\d+\.\d+\.\d+)/)?.[1];

    const frontmatter = {
        title: version,
        description: summary.replace(/[*_`[\]]/g, '').replace(/\(https?:\/\/[^)]+\)/g, ''),
        version: version.slice(1),
        major: Number(version.slice(1).split('.')[0]),
        order: sections.length - index,
        date: dates.get(version) ?? '',
        compare: compare?.[1] ?? '',
        desktop: desktop ?? '',
    };

    const yaml = Object.entries(frontmatter)
        .map(([key, value]) => `${key}: ${JSON.stringify(value)}`)
        .join('\n');

    writeFileSync(
        `${output}${version}.md`,
        `---\n${yaml}\n---\n\n${body.replace(/^### /gm, '## ').replace(/^#### /gm, '### ')}\n`,
    );
});

console.log(`Wrote ${sections.length} releases to ${output}`);
