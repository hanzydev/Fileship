import { filesize } from 'filesize';

export const useDiscordPreview = () => {
    const { t, locale } = useI18n();
    const date = useDate();

    const defaultLayout = (kind: DiscordEmbedKind) =>
        defaultDiscordLayout(kind, { download: t('linkPreview.download') });

    const values = (kind: DiscordEmbedKind, embed: IEmbed) => {
        const classic: Record<string, string> = {
            fileName: 'holiday.png',
            mimeType: 'image/png',
            size: filesize(2_400_000, { locale: locale.value }),
            createdAt: date().format('LLL'),
            now: date().format('LLL'),
        };

        return {
            ...DISCORD_SAMPLE_VALUES,
            ...classic,
            name: kind === 'folder' ? t('sharing.discord.sampleFolder') : 'holiday.png',
            title:
                kind === 'note'
                    ? t('sharing.discord.sampleNote')
                    : fillEmbedText(embed.title, classic) || 'holiday.png',
            description: fillEmbedText(embed.description, classic),
            siteName: fillEmbedText(embed.siteName, classic),
            excerpt: t('sharing.discord.sampleExcerpt'),
            files: t('linkPreview.files', 12),
            folders: t('linkPreview.folders', 3),
        };
    };

    const preview = (kind: DiscordEmbedKind, embed: IEmbed, layout?: DiscordLayout) =>
        buildDiscordEmbed(
            layout ?? embed.discord[kind] ?? defaultLayout(kind),
            values(kind, embed),
            embed.color,
        );

    return { defaultLayout, preview };
};
