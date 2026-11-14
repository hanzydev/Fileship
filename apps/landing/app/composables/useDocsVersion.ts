export interface DocsVersion {
    key: 'stable' | 'experimental';
    version: string;
    root: string;
    collection: 'docs' | 'docsExperimental';
    branch: 'main' | 'next';
}

export const useDocsVersion = () => {
    const route = useRoute();
    const { docsVersions } = useAppConfig();

    const versions: DocsVersion[] = [
        {
            key: 'stable',
            version: docsVersions.stable,
            root: '/docs',
            collection: 'docs',
            branch: 'main',
        },
    ];

    if (docsVersions.experimental)
        versions.push({
            key: 'experimental',
            version: docsVersions.experimental,
            root: '/docs/experimental',
            collection: 'docsExperimental',
            branch: 'next',
        });

    const version = computed(() =>
        versions.findLast(
            (item) => route.path === item.root || route.path.startsWith(`${item.root}/`),
        )!,
    );

    return { versions, version };
};
