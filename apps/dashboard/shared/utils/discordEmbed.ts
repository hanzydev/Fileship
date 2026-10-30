import { z } from 'zod';

import { defaultEmbed } from './constants';
import type { IEmbed } from './types';

export type DiscordEmbedKind = 'file' | 'note' | 'folder';

export const DISCORD_EMBED_KINDS: DiscordEmbedKind[] = ['file', 'note', 'folder'];

export const DISCORD_COMPONENT_LIMIT = 40;

const DISCORD_EMBED_MAX_BYTES = 3_000;

export const DISCORD_PLACEHOLDERS: Record<DiscordEmbedKind, string[]> = {
    file: [
        'title',
        'description',
        'name',
        'fileName',
        'mimeType',
        'size',
        'url',
        'fileUrl',
        'downloadUrl',
        'thumbnailUrl',
        'siteName',
        'createdAt',
        'now',
    ],
    note: ['title', 'excerpt', 'url', 'siteName', 'createdAt', 'now'],
    folder: [
        'name',
        'files',
        'folders',
        'fileCount',
        'folderCount',
        'url',
        'siteName',
        'createdAt',
        'now',
    ],
};

const URL_PLACEHOLDERS = new Set(['url', 'fileUrl', 'downloadUrl', 'thumbnailUrl']);

const text = z.string().max(2_000);
const url = z.string().trim().max(2_048);

const buttonSchema = z
    .object({
        label: z.string().trim().max(80),
        url,
        emoji: z.string().trim().max(32),
    })
    .strict();

const mediaSchema = z
    .object({
        url,
        description: z.string().max(1_024),
        spoiler: z.boolean(),
    })
    .strict();

export const discordChildSchema = z.discriminatedUnion('type', [
    z.object({ type: z.literal('text'), content: text }).strict(),
    z
        .object({
            type: z.literal('section'),
            texts: z.array(text).min(1).max(3),
            accessory: z.discriminatedUnion('type', [
                mediaSchema.extend({ type: z.literal('thumbnail') }).strict(),
                buttonSchema.extend({ type: z.literal('button') }).strict(),
            ]),
        })
        .strict(),
    z.object({ type: z.literal('gallery'), items: z.array(mediaSchema).min(1).max(10) }).strict(),
    z.object({ type: z.literal('separator'), divider: z.boolean(), large: z.boolean() }).strict(),
    z.object({ type: z.literal('buttons'), buttons: z.array(buttonSchema).min(1).max(5) }).strict(),
]);

export type DiscordChild = z.infer<typeof discordChildSchema>;
export type DiscordButton = z.infer<typeof buttonSchema>;
export type DiscordMedia = z.infer<typeof mediaSchema>;

export const discordComponentCount = (components: DiscordChild[]) =>
    components.reduce((total, component) => {
        if (component.type === 'section') return total + 2 + component.texts.length;
        if (component.type === 'buttons') return total + 1 + component.buttons.length;
        return total + 1;
    }, 1);

export const discordLayoutSchema = z
    .object({
        spoiler: z.boolean(),
        components: z
            .array(discordChildSchema)
            .min(1)
            .max(DISCORD_COMPONENT_LIMIT - 1),
    })
    .strict()
    .refine((layout) => discordComponentCount(layout.components) <= DISCORD_COMPONENT_LIMIT);

export type DiscordLayout = z.infer<typeof discordLayoutSchema>;

export const discordSettingsSchema = z
    .object({
        enabled: z.boolean(),
        file: discordLayoutSchema.nullable(),
        note: discordLayoutSchema.nullable(),
        folder: discordLayoutSchema.nullable(),
    })
    .strict();

export const embedOf = (raw: unknown): IEmbed => {
    const value = (raw && typeof raw === 'object' ? raw : {}) as Record<string, any>;
    const discord = value.discord && typeof value.discord === 'object' ? value.discord : {};
    const pick = <T>(schema: z.ZodType<T>, input: unknown, fallback: T) => {
        const result = schema.safeParse(input);
        return result.success ? result.data : fallback;
    };

    return {
        enabled: pick(z.boolean(), value.enabled, defaultEmbed.enabled),
        color: pick(z.string().regex(/^#[\da-f]{6}$/i), value.color, defaultEmbed.color),
        title: pick(z.string(), value.title, defaultEmbed.title),
        description: pick(z.string(), value.description, defaultEmbed.description),
        siteName: pick(z.string(), value.siteName, defaultEmbed.siteName),
        discord: {
            enabled: pick(z.boolean(), discord.enabled, defaultEmbed.discord.enabled),
            file: pick(discordLayoutSchema.nullable(), discord.file, null),
            note: pick(discordLayoutSchema.nullable(), discord.note, null),
            folder: pick(discordLayoutSchema.nullable(), discord.folder, null),
        },
    };
};

export const defaultDiscordLayout = (
    kind: DiscordEmbedKind,
    labels: { download: string },
): DiscordLayout => {
    if (kind === 'file')
        return {
            spoiler: false,
            components: [
                { type: 'text', content: '-# {siteName}\n## [{title}]({url})\n{description}' },
                {
                    type: 'gallery',
                    items: [{ url: '{fileUrl}', description: '{name}', spoiler: false }],
                },
                {
                    type: 'buttons',
                    buttons: [{ label: labels.download, url: '{downloadUrl}', emoji: '' }],
                },
            ],
        };

    if (kind === 'note')
        return {
            spoiler: false,
            components: [
                { type: 'text', content: '-# {siteName}\n## [{title}]({url})\n{excerpt}' },
            ],
        };

    return {
        spoiler: false,
        components: [
            { type: 'text', content: '-# {siteName}\n## [{name}]({url})\n{files} · {folders}' },
        ],
    };
};

export const escapeDiscord = (value: string) => value.replace(/[\\*_~`|>[\]<]/g, '\\$&');

const PLACEHOLDER = /\{(\w+)\}/g;

const fillUrl = (template: string, values: Record<string, string>) =>
    template.replace(PLACEHOLDER, (match, name) => values[name] ?? match).trim();

const fillText = (template: string, values: Record<string, string>) =>
    template
        .split('\n')
        .flatMap((line) => {
            const filled = line.replace(PLACEHOLDER, (match, name) => {
                const value = values[name];
                if (value === undefined) return match;
                return URL_PLACEHOLDERS.has(name)
                    ? value.replace(/\(/g, '%28').replace(/\)/g, '%29')
                    : escapeDiscord(value);
            });

            const isEmpty =
                filled !== line &&
                !filled
                    .replace(/^(-#|#{1,3}|>)\s*/, '')
                    .replace(/\[\]\([^)]*\)/g, '')
                    .replace(/[\s·•|-]/g, '');

            return isEmpty ? [] : [filled];
        })
        .join('\n')
        .trim();

const isHttpUrl = (value: string) => {
    try {
        return ['http:', 'https:'].includes(new URL(value).protocol);
    } catch {
        return false;
    }
};

const buildButton = (button: DiscordButton, values: Record<string, string>) => {
    const label = fillUrl(button.label, values).slice(0, 80);
    const target = fillUrl(button.url, values);

    if (!isHttpUrl(target) || (!label && !button.emoji)) return null;

    return {
        type: 2,
        style: 5,
        url: target,
        ...(label && { label }),
        ...(button.emoji && { emoji: { name: button.emoji } }),
    };
};

const buildMedia = (media: DiscordMedia, values: Record<string, string>) => {
    const target = fillUrl(media.url, values);
    if (!isHttpUrl(target)) return null;

    const description = fillUrl(media.description, values).slice(0, 1_024);

    return {
        media: { url: target },
        ...(description && { description }),
        ...(media.spoiler && { spoiler: true }),
    };
};

const buildChild = (child: DiscordChild, values: Record<string, string>): object[] => {
    switch (child.type) {
        case 'text': {
            const content = fillText(child.content, values);
            return content ? [{ type: 10, content }] : [];
        }

        case 'section': {
            const texts = child.texts
                .map((content) => fillText(content, values))
                .filter(Boolean)
                .map((content) => ({ type: 10, content }));

            if (!texts.length) return [];

            const accessory =
                child.accessory.type === 'thumbnail'
                    ? buildMedia(child.accessory, values)
                    : buildButton(child.accessory, values);

            if (!accessory) return texts;

            return [
                {
                    type: 9,
                    components: texts,
                    accessory:
                        child.accessory.type === 'thumbnail'
                            ? { type: 11, ...accessory }
                            : accessory,
                },
            ];
        }

        case 'gallery': {
            const items = child.items.flatMap((item) => buildMedia(item, values) ?? []);
            return items.length ? [{ type: 12, items }] : [];
        }

        case 'separator':
            return [
                {
                    type: 14,
                    ...(!child.divider && { divider: false }),
                    ...(child.large && { spacing: 2 }),
                },
            ];

        case 'buttons': {
            const buttons = child.buttons.flatMap((button) => buildButton(button, values) ?? []);
            return buttons.length ? [{ type: 1, components: buttons }] : [];
        }
    }
};

export const buildDiscordEmbed = (
    layout: DiscordLayout,
    values: Record<string, string>,
    color: string,
) => {
    const components = layout.components.flatMap((child) => buildChild(child, values));

    while (components[0] && (components[0] as { type: number }).type === 14) components.shift();
    while (components.at(-1) && (components.at(-1) as { type: number }).type === 14)
        components.pop();

    if (!components.length) return null;

    return {
        component: {
            type: 17,
            accent_color: Number.parseInt(color.slice(1), 16),
            ...(layout.spoiler && { spoiler: true }),
            components,
        },
    };
};

export const serializeDiscordEmbed = (
    layout: DiscordLayout,
    values: Record<string, string>,
    color: string,
) => {
    const embed = buildDiscordEmbed(layout, values, color);
    if (!embed) return null;

    const json = JSON.stringify(embed)
        .replaceAll('<', '\\u003c')
        .replaceAll('>', '\\u003e')
        .replaceAll('&', '\\u0026')
        .replaceAll(' ', '\\u2028')
        .replaceAll(' ', '\\u2029');

    return new TextEncoder().encode(json).byteLength <= DISCORD_EMBED_MAX_BYTES ? json : null;
};

export const fillEmbedText = (template: string, values: Record<string, string>) =>
    template.replace(PLACEHOLDER, (match, name) => values[name] ?? match);

export const DISCORD_SAMPLE_VALUES: Record<string, string> = {
    title: 'holiday.png',
    description: 'A sunny day at the beach',
    name: 'holiday.png',
    fileName: 'holiday.png',
    mimeType: 'image/png',
    size: '2.4 MB',
    url: 'https://example.com/view/holiday.png',
    fileUrl: 'https://example.com/u/holiday.png',
    downloadUrl: 'https://example.com/u/holiday.png?download',
    thumbnailUrl: 'https://example.com/u/holiday.png/thumbnail',
    excerpt: 'Shopping list: milk, eggs, bread',
    files: '12 files',
    folders: '3 folders',
    fileCount: '12',
    folderCount: '3',
    siteName: 'Fileship',
    createdAt: 'October 30, 2026 at 12:00',
    now: 'October 30, 2026 at 12:00',
};
