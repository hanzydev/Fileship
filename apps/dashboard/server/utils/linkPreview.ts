import { filesize } from 'filesize';

import { linkPreview as az } from '~~/i18n/locales/az.json';
import { linkPreview as de } from '~~/i18n/locales/de.json';
import { linkPreview as en } from '~~/i18n/locales/en.json';
import { linkPreview as es } from '~~/i18n/locales/es.json';
import { linkPreview as fr } from '~~/i18n/locales/fr.json';
import { linkPreview as it } from '~~/i18n/locales/it.json';
import { linkPreview as ja } from '~~/i18n/locales/ja.json';
import { linkPreview as ko } from '~~/i18n/locales/ko.json';
import { linkPreview as pt } from '~~/i18n/locales/pt.json';
import { linkPreview as ru } from '~~/i18n/locales/ru.json';
import { linkPreview as tr } from '~~/i18n/locales/tr.json';
import { linkPreview as zh } from '~~/i18n/locales/zh.json';

const translator = catalogTranslator<keyof typeof en>({
    en,
    tr,
    de,
    es,
    fr,
    it,
    pt,
    ru,
    az,
    ja,
    ko,
    zh,
});

const formatDate = (date: Date, language: string) =>
    new Intl.DateTimeFormat(language, { dateStyle: 'long', timeStyle: 'short' }).format(date);

type PreviewValues = {
    file: {
        name: string;
        fileName: string;
        mimeType: string;
        size: bigint;
        url: string;
        fileUrl: string;
        downloadUrl: string;
        thumbnailUrl: string;
    };
    note: { title: string; excerpt: string; url: string };
    folder: { name: string; fileCount: number; folderCount: number; url: string };
};

export const noteExcerpt = (content: string) => {
    const plain = content
        .replace(/```[\s\S]*?```/g, ' ')
        .replace(/!?\[([^\]]*)\]\([^)]*\)/g, '$1')
        .replace(/[#>*_`~|-]+/g, ' ')
        .replace(/\s+/g, ' ')
        .trim();

    return plain.length > 300 ? `${plain.slice(0, 299)}…` : plain;
};

export const discordEmbedFor = <Kind extends DiscordEmbedKind>(
    kind: Kind,
    owner: { embed: unknown; language: string | null },
    values: PreviewValues[Kind],
    createdAt: Date,
) => {
    const embed = embedOf(owner.embed);
    if (!embed.enabled || !embed.discord.enabled) return null;

    const language = owner.language ?? 'en';
    const t = translator(language);

    const common: Record<string, string> = {
        createdAt: formatDate(createdAt, language),
        now: formatDate(new Date(), language),
    };

    const filled: Record<string, string> =
        kind === 'folder'
            ? (() => {
                  const folder = values as PreviewValues['folder'];
                  return {
                      ...common,
                      name: folder.name,
                      url: folder.url,
                      fileCount: String(folder.fileCount),
                      folderCount: String(folder.folderCount),
                      files: t('files', { count: folder.fileCount }),
                      folders: t('folders', { count: folder.folderCount }),
                  };
              })()
            : { ...common, ...(values as Record<string, string>) };

    if (kind === 'file') {
        filled.size = filesize((values as PreviewValues['file']).size, { locale: language });
        filled.title = fillEmbedText(embed.title, filled) || filled.name!;
        filled.description = fillEmbedText(embed.description, filled);
    }

    filled.siteName = fillEmbedText(embed.siteName, filled);

    const layout = embed.discord[kind] ?? defaultDiscordLayout(kind, { download: t('download') });

    return serializeDiscordEmbed(layout, filled, embed.color);
};
