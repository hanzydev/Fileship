export const COMPARE_APPS = ['Fileship', 'Zipline', 'XBackBone', 'Chibisafe'];

export const useCompareRows = () => {
    const { t } = useI18n();

    const yes = (note = '') => ({ kind: 'yes', note });
    const no = (note = '') => ({ kind: 'no', note });
    const part = (note = '') => ({ kind: 'part', note });
    const text = (note = '') => ({ kind: 'text', note });

    const rows = computed(() => {
        const note = (key = '') => t(`compare.notes.${key}`);
        const local = note('local');

        return [
            { key: 'uploaders', cells: [yes(), yes(), yes(), yes()] },
            { key: 'desktop', cells: [yes('Windows'), no(), no(), no()] },
            { key: 'explorer', cells: [yes('Harbor'), no(), no(), no()] },
            { key: 'extension', cells: [yes(), no(), no(), yes()] },
            { key: 'ai', cells: [yes(note('onYourServer')), no(), no(), no()] },
            {
                key: 'folders',
                cells: [
                    yes(note('nestedShared')),
                    yes(note('nested')),
                    no(note('tags')),
                    part(note('flatAlbums')),
                ],
            },
            { key: 'sharing', cells: [yes(), yes(), no(), part(note('albumLinks'))] },
            {
                key: 'storage',
                cells: [
                    text(`${local}, S3, SFTP, FTP`),
                    text(`${local}, S3`),
                    text(`${local}, S3, FTP, +2`),
                    text(`${local}, S3`),
                ],
            },
            { key: 'security', cells: [yes(), yes(), no(), no()] },
            { key: 'sso', cells: [yes('OIDC'), yes('OAuth, OIDC'), yes('LDAP'), no()] },
            {
                key: 'webhooks',
                cells: [yes('Discord, Slack, JSON'), yes('Discord, HTTP'), no(), no()],
            },
            { key: 'backups', cells: [yes(), part(note('export')), part(note('export')), no()] },
            {
                key: 'updates',
                cells: [yes(note('zeroDowntime')), no(), part(note('withDowntime')), no()],
            },
            {
                key: 'languages',
                cells: [text('12'), text('12'), text(note('somePartial')), text('1')],
            },
            {
                key: 'license',
                cells: [text('AGPL-3.0'), text('MIT'), text('AGPL-3.0'), text('MIT')],
            },
        ];
    });

    return rows;
};
