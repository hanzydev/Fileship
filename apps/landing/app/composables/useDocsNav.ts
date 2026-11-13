export interface DocsNavPage {
    path: string;
    title: string;
}

const groupOf = (path: string) => {
    const [, , folder, ...rest] = path.split('/');
    const group = rest.length ? DOCS_NAV.find((g) => g.folders.includes(folder!)) : undefined;

    return group?.key ?? 'more';
};

export const useDocsNav = async () => {
    const { version } = useDocsVersion();

    const { data: pages } = await useAsyncData(
        () => `docs-nav-${version.value.collection}`,
        () => queryCollection(version.value.collection).select('path', 'title').all(),
    );

    const groups = computed(() => {
        const known = new Map(
            (pages.value ?? []).map((page) => [
                `/docs${page.path.slice(version.value.root.length)}`,
                page,
            ]),
        );
        const listed = new Set(DOCS_NAV.flatMap((group) => group.items));

        return DOCS_NAV.map((group) => {
            const ordered = group.items.flatMap((path) => known.get(path) ?? []);
            const extra = [...known.values()]
                .filter((page) => !listed.has(page.path))
                .filter((page) => groupOf(page.path) === group.key)
                .sort((a, b) => a.title.localeCompare(b.title));

            return { key: group.key, pages: [...ordered, ...extra] };
        }).filter((group) => group.pages.length);
    });

    const flat = computed(() => groups.value.flatMap((group) => group.pages));

    return { groups, flat };
};
