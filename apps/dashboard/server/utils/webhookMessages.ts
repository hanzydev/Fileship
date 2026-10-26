import { filesize } from 'filesize';

import { pluralIndex } from '@fileship/ui/i18n/plural';

import { webhookMessages as az } from '~~/i18n/locales/az.json';
import { webhookMessages as de } from '~~/i18n/locales/de.json';
import { webhookMessages as en } from '~~/i18n/locales/en.json';
import { webhookMessages as fr } from '~~/i18n/locales/fr.json';
import { webhookMessages as it } from '~~/i18n/locales/it.json';
import { webhookMessages as ja } from '~~/i18n/locales/ja.json';
import { webhookMessages as ko } from '~~/i18n/locales/ko.json';
import { webhookMessages as tr } from '~~/i18n/locales/tr.json';
import { webhookMessages as zh } from '~~/i18n/locales/zh.json';

type MessageKey = keyof typeof en;
type Params = Record<string, string | number>;

type Field = { name: string; value?: string; date?: string; link?: string };

type Card = {
    heading: string;
    title?: string;
    url?: string | null;
    description?: string;
    lines?: string[];
    fields?: Field[];
    thumbnail?: string | null;
};

const MESSAGES: Record<string, Record<MessageKey, string>> = { en, tr, de, fr, it, az, ja, ko, zh };

const translator =
    (language: string) =>
    (key: MessageKey, params: Params = {}) => {
        const choices = (MESSAGES[language] ?? en)[key].split(' | ');
        const count = typeof params.count === 'number' ? params.count : undefined;

        const message =
            count === undefined || choices.length === 1
                ? choices[0]!
                : choices[pluralIndex(language, count, choices.length)]!;

        return message
            .replace(/\{'([^']*)'\}/g, '$1')
            .replace(/\{(\w+)\}/g, (match, name) => String(params[name] ?? match));
    };

const LIST_LIMIT = 10;

const DELETE_REASONS: Record<WebhookDeleteReason, MessageKey> = {
    trashed: 'filesTrashed',
    deleted: 'filesDeleted',
    expired: 'filesExpired',
    maxViews: 'filesMaxViews',
};

const truncate = (value: string, length: number) =>
    value.length > length ? `${value.slice(0, length - 1)}…` : value;

const accentColor = (theme: string) => {
    const [l, c, h] = (accents[theme as Accent] ?? accents.Fileship)
        .match(/[\d.]+/g)!
        .map(Number) as [number, number, number];

    const a = c * Math.cos((h * Math.PI) / 180);
    const b = c * Math.sin((h * Math.PI) / 180);

    const lms = [
        l + 0.3963377774 * a + 0.2158037573 * b,
        l - 0.1055613458 * a - 0.0638541728 * b,
        l - 0.0894841775 * a - 1.291485548 * b,
    ].map((value) => value ** 3) as [number, number, number];

    const channel = (value: number) => {
        const linear = Math.min(1, Math.max(0, value));
        return Math.round(
            255 * (linear <= 0.0031308 ? 12.92 * linear : 1.055 * linear ** (1 / 2.4) - 0.055),
        );
    };

    const [r, g, bl] = [
        4.0767416621 * lms[0] - 3.3077115913 * lms[1] + 0.2309699292 * lms[2],
        -1.2684380046 * lms[0] + 2.6097574011 * lms[1] - 0.3413193965 * lms[2],
        -0.0041960863 * lms[0] - 0.7034186147 * lms[1] + 1.707614701 * lms[2],
    ].map(channel) as [number, number, number];

    return (r << 16) | (g << 8) | bl;
};

const buildCard = (payload: WebhookPayload, language: string): Card => {
    const t = translator(language);
    const data = payload.data as any;
    const size = (value: number) => filesize(value, { locale: language });
    const quotaSize = { locale: language, base: 2, standard: 'jedec' } as const;

    const access = (visibility: WebhookVisibility): Field[] =>
        visibility === 'public'
            ? []
            : [
                  {
                      name: t('access'),
                      value: t(
                          visibility === 'locked' ? 'passwordProtected' : 'hiddenFromVisitors',
                      ),
                  },
              ];

    switch (payload.event) {
        case 'file.uploaded':
            return {
                heading: t(data.source === 'inbox' ? 'inboxUpload' : 'fileUploaded'),
                title: data.file.name,
                url: data.file.viewUrl,
                fields: [
                    { name: t('size'), value: size(data.file.size) },
                    { name: t('type'), value: data.file.mimeType || '-' },
                    ...access(data.file.visibility),
                ],
                thumbnail: data.file.thumbnailUrl,
            };

        case 'file.deleted': {
            const heading = t(DELETE_REASONS[data.reason as WebhookDeleteReason], {
                count: data.count,
            });

            if (data.count === 1)
                return {
                    heading,
                    title: data.files[0].name,
                    fields: [{ name: t('size'), value: size(data.files[0].size) }],
                };

            const shown = data.files.slice(0, LIST_LIMIT);

            return {
                heading,
                lines: [
                    ...shown.map((file: { name: string }) => file.name),
                    ...(data.count > shown.length
                        ? [t('moreFiles', { count: data.count - shown.length })]
                        : []),
                ],
            };
        }

        case 'link.created':
            return {
                heading: t('linkCreated'),
                title: data.link.url,
                url: data.link.url,
                fields: [
                    data.link.target
                        ? { name: t('destination'), link: data.link.target }
                        : { name: t('destination'), value: t('passwordProtected') },
                    ...(data.link.expiresAt
                        ? [{ name: t('expires'), date: data.link.expiresAt }]
                        : []),
                    ...(data.link.maxClicks
                        ? [{ name: t('maxClicks'), value: String(data.link.maxClicks) }]
                        : []),
                ],
            };

        case 'note.created':
            return {
                heading: t('noteCreated'),
                title: data.note.title ?? t('privateNote'),
                url: data.note.url,
            };

        case 'storage.almostFull':
            return {
                heading: t('storageAlmostFull'),
                title: t('storageUsage', {
                    percent: new Intl.NumberFormat(language).format(data.storage.percent),
                }),
                url: data.storage.url,
                description: t('storageUsageDescription', {
                    used: filesize(data.storage.used, quotaSize),
                    limit: filesize(data.storage.limit, quotaSize),
                }),
            };

        default:
            return {
                heading: t('test'),
                title: t('testTitle'),
                description: t('testDescription'),
            };
    }
};

const escapeDiscord = (value: string) => value.replace(/[\\*_~`|>[\]<]/g, '\\$&');

const unix = (date: string) => Math.floor(new Date(date).getTime() / 1000);

const discordMessage = (card: Card, payload: WebhookPayload, color: number, footer: string) => ({
    allowed_mentions: { parse: [] },
    embeds: [
        {
            author: { name: truncate(card.heading, 256) },
            ...(card.title && { title: escapeDiscord(truncate(card.title, 200)) }),
            ...(card.url && { url: card.url }),
            ...((card.description || card.lines) && {
                description: [
                    ...(card.description ? [escapeDiscord(card.description)] : []),
                    ...(card.lines ?? []).map((line) => `• ${escapeDiscord(truncate(line, 200))}`),
                ].join('\n'),
            }),
            color,
            ...(card.fields?.length && {
                fields: card.fields.map((field) => ({
                    name: field.name,
                    value: field.date
                        ? `<t:${unix(field.date)}:f>`
                        : (field.link ?? escapeDiscord(truncate(field.value!, 900))),
                    inline: true,
                })),
            }),
            ...(card.thumbnail && { thumbnail: { url: card.thumbnail } }),
            footer: { text: truncate(footer, 256) },
            timestamp: payload.createdAt,
        },
    ],
});

const escapeSlack = (value: string) =>
    value.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');

const slackMessage = (card: Card, footer: string) => {
    const title = card.title && escapeSlack(truncate(card.title, 200));

    return {
        text: escapeSlack(card.title ? `${card.heading}: ${card.title}` : card.heading),
        blocks: [
            {
                type: 'context',
                elements: [{ type: 'mrkdwn', text: `*${escapeSlack(card.heading)}*` }],
            },
            ...(title || card.description || card.lines
                ? [
                      {
                          type: 'section',
                          text: {
                              type: 'mrkdwn',
                              text: [
                                  ...(title
                                      ? [card.url ? `*<${card.url}|${title}>*` : `*${title}*`]
                                      : []),
                                  ...(card.description ? [escapeSlack(card.description)] : []),
                                  ...(card.lines ?? []).map(
                                      (line) => `• ${escapeSlack(truncate(line, 200))}`,
                                  ),
                              ].join('\n'),
                          },
                          ...(card.thumbnail && {
                              accessory: {
                                  type: 'image',
                                  image_url: card.thumbnail,
                                  alt_text: truncate(card.title ?? card.heading, 200),
                              },
                          }),
                      },
                  ]
                : []),
            ...(card.fields?.length
                ? [
                      {
                          type: 'section',
                          fields: card.fields.map((field) => ({
                              type: 'mrkdwn',
                              text: `*${escapeSlack(field.name)}*\n${
                                  field.date
                                      ? `<!date^${unix(field.date)}^{date_short_pretty} {time}|${field.date}>`
                                      : escapeSlack(field.link ?? truncate(field.value!, 900))
                              }`,
                          })),
                      },
                  ]
                : []),
            { type: 'context', elements: [{ type: 'mrkdwn', text: escapeSlack(footer) }] },
        ],
    };
};

export const formatWebhookMessage = (
    format: 'Discord' | 'Slack',
    payload: WebhookPayload,
    options: { language: string; theme: string; siteName: string },
) => {
    const card = buildCard(payload, options.language);

    return format === 'Discord'
        ? discordMessage(card, payload, accentColor(options.theme), options.siteName)
        : slackMessage(card, options.siteName);
};
