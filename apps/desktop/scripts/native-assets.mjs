import { mkdirSync, readdirSync, readFileSync, writeFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname, join } from 'node:path';
import { pathToFileURL } from 'node:url';

import { root } from './msvc.mjs';

const workspace = join(root, '..', '..');
const generated = join(root, 'build', 'generated');
const catalogs = join(root, 'build', 'host', 'i18n');

const THEME_TOKENS = [
    'background',
    'foreground',
    'card',
    'card-foreground',
    'popover',
    'popover-foreground',
    'primary',
    'primary-foreground',
    'secondary',
    'secondary-foreground',
    'muted',
    'muted-foreground',
    'accent',
    'accent-foreground',
    'destructive',
    'border',
    'input',
    'ring',
];

const ICONS = [
    'circle',
    'droplets',
    'eraser',
    'grid-3x3',
    'grip-horizontal',
    'grip-vertical',
    'highlighter',
    'list-ordered',
    'loader-circle',
    'mic',
    'mic-off',
    'minus',
    'mouse-pointer-2',
    'move-up-right',
    'pause',
    'pencil',
    'pipette',
    'play',
    'redo-2',
    'scan',
    'sliders-horizontal',
    'smile',
    'square',
    'trash-2',
    'type',
    'undo-2',
    'volume-2',
    'volume-x',
    'x',
];

const camelCase = (name) => name.replace(/-(\w)/g, (_, letter) => letter.toUpperCase());

const toSrgb = (linear) => {
    const value = linear <= 0.0031308 ? 12.92 * linear : 1.055 * linear ** (1 / 2.4) - 0.055;
    return Math.min(1, Math.max(0, value));
};

const oklchToSrgb = (lightness, chroma, hue) => {
    const angle = (hue * Math.PI) / 180;
    const a = chroma * Math.cos(angle);
    const b = chroma * Math.sin(angle);

    const l = (lightness + 0.3963377774 * a + 0.2158037573 * b) ** 3;
    const m = (lightness - 0.1055613458 * a - 0.0638541728 * b) ** 3;
    const s = (lightness - 0.0894841775 * a - 1.291485548 * b) ** 3;

    return [
        toSrgb(4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s),
        toSrgb(-1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s),
        toSrgb(-0.0041960863 * l - 0.7034186147 * m + 1.707614701 * s),
    ];
};

const readBlock = (css, selector) => {
    const start = css.indexOf(`${selector} {`);
    return css.slice(start, css.indexOf('}', start));
};

const readColor = (block, token) => {
    const match = block.match(
        new RegExp(
            `--${token}:\\s*oklch\\(([\\d.]+)\\s+([\\d.]+)\\s+([\\d.]+)(?:\\s*/\\s*([\\d.]+)%)?\\)`,
        ),
    );
    if (!match) throw new Error(`The theme token --${token} is missing or not oklch.`);

    const [, lightness, chroma, hue, alpha] = match;
    const [r, g, b] = oklchToSrgb(Number(lightness), Number(chroma), Number(hue));
    const opacity = alpha === undefined ? 1 : Number(alpha) / 100;

    return `{${[r, g, b, opacity].map((value) => `${value.toFixed(6)}f`).join(', ')}}`;
};

const writeTheme = () => {
    const css = readFileSync(
        join(workspace, 'packages', 'ui', 'app', 'styles', 'tailwind.css'),
        'utf8',
    );
    const light = readBlock(css, ':root');
    const dark = readBlock(css, '.dark');
    const radius = Number(light.match(/--radius:\s*([\d.]+)rem/)[1]) * 16;

    const theme = (block) =>
        THEME_TOKENS.map((token) => `    ${readColor(block, token)},`).join('\n');

    writeFileSync(
        join(generated, 'theme.h'),
        [
            '#pragma once',
            '',
            'namespace fileship {',
            '',
            'struct ThemeColor {',
            '    float r, g, b, a;',
            '};',
            '',
            'struct Theme {',
            ...THEME_TOKENS.map((token) => `    ThemeColor ${camelCase(token)};`),
            '};',
            '',
            `inline constexpr float THEME_RADIUS = ${radius.toFixed(1)}f;`,
            '',
            'inline constexpr Theme LIGHT_THEME{',
            theme(light),
            '};',
            '',
            'inline constexpr Theme DARK_THEME{',
            theme(dark),
            '};',
            '',
            '}',
            '',
        ].join('\n'),
    );
};

const number = (value) => Number(value ?? 0);

const toPath = ([tag, attributes]) => {
    if (tag === 'path') return attributes.d;

    if (tag === 'line') {
        return `M${attributes.x1} ${attributes.y1}L${attributes.x2} ${attributes.y2}`;
    }

    if (tag === 'polyline' || tag === 'polygon') {
        const [first, ...rest] = attributes.points.trim().split(/\s+/);
        return `M${first}${rest.map((point) => `L${point}`).join('')}${tag === 'polygon' ? 'Z' : ''}`;
    }

    if (tag === 'circle' || tag === 'ellipse') {
        const cx = number(attributes.cx);
        const cy = number(attributes.cy);
        const rx = number(attributes.rx ?? attributes.r);
        const ry = number(attributes.ry ?? attributes.r);

        return `M${cx - rx} ${cy}a${rx} ${ry} 0 1 0 ${rx * 2} 0a${rx} ${ry} 0 1 0 ${-rx * 2} 0Z`;
    }

    if (tag === 'rect') {
        const x = number(attributes.x);
        const y = number(attributes.y);
        const width = number(attributes.width);
        const height = number(attributes.height);
        const r = Math.min(number(attributes.rx ?? attributes.ry), width / 2, height / 2);

        if (!r) return `M${x} ${y}h${width}v${height}h${-width}Z`;

        return [
            `M${x + r} ${y}h${width - r * 2}a${r} ${r} 0 0 1 ${r} ${r}`,
            `v${height - r * 2}a${r} ${r} 0 0 1 ${-r} ${r}`,
            `h${-(width - r * 2)}a${r} ${r} 0 0 1 ${-r} ${-r}`,
            `v${-(height - r * 2)}a${r} ${r} 0 0 1 ${r} ${-r}Z`,
        ].join('');
    }

    throw new Error(`Unsupported icon element <${tag}>.`);
};

const writeIcons = async () => {
    const require = createRequire(join(workspace, 'packages', 'ui', 'package.json'));
    const icons = join(
        dirname(require.resolve('@lucide/vue/package.json')),
        'dist',
        'esm',
        'icons',
    );
    const files = new Map();

    for (const file of readdirSync(icons).filter((name) => name.endsWith('.mjs'))) {
        const source = readFileSync(join(icons, file), 'utf8');
        const aliases = source.match(/aliases:\s*\[([^\]]*)\]/)?.[1].match(/"[^"]+"/g) ?? [];

        files.set(file.slice(0, -4), file);
        for (const alias of aliases) {
            if (!files.has(JSON.parse(alias))) files.set(JSON.parse(alias), file);
        }
    }

    const lines = [
        '#pragma once',
        '',
        '#include <cstddef>',
        '',
        'namespace fileship {',
        '',
        'struct Icon {',
        '    char const* const* paths;',
        '    size_t count;',
        '};',
        '',
        'namespace icons {',
        '',
    ];

    for (const name of ICONS) {
        const file = files.get(name);
        if (!file) throw new Error(`The lucide icon ${name} does not exist.`);

        const { __iconData } = await import(pathToFileURL(join(icons, file)).href);
        const id = camelCase(name);
        const paths = __iconData.node.map(toPath);

        lines.push(
            `inline constexpr char const* ${id}Paths[] = {`,
            ...paths.map((path) => `    ${JSON.stringify(path)},`),
            '};',
            `inline constexpr Icon ${id}{${id}Paths, ${paths.length}};`,
            '',
        );
    }

    lines.push('}', '', '}', '');
    writeFileSync(join(generated, 'icons.h'), lines.join('\n'));
};

const merge = (target, source) => {
    for (const [key, value] of Object.entries(source)) {
        target[key] =
            value && typeof value === 'object' && !Array.isArray(value)
                ? merge(target[key] ?? {}, value)
                : value;
    }

    return target;
};

const writeCatalogs = () => {
    const desktop = join(root, 'i18n', 'locales');
    const layer = join(workspace, 'packages', 'ui', 'i18n', 'locales');

    for (const file of readdirSync(desktop).filter((name) => name.endsWith('.json'))) {
        const messages = merge(
            JSON.parse(readFileSync(join(layer, file), 'utf8')),
            JSON.parse(readFileSync(join(desktop, file), 'utf8')),
        );

        writeFileSync(join(catalogs, file), JSON.stringify(messages));
    }
};

mkdirSync(generated, { recursive: true });
mkdirSync(catalogs, { recursive: true });

writeTheme();
await writeIcons();
writeCatalogs();
