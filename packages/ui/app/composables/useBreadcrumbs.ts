import { upperFirst } from 'scule';

export const useBreadcrumbs = (staticLabels: Record<string, string>) => {
    const route = useRoute();
    const labels = useBreadcrumbLabels();
    const trail = useBreadcrumbTrail();
    const { t } = useI18n();

    return computed(() => {
        const segments = route.path.split('/').filter(Boolean);
        let path = '';

        const items = segments.map((segment) => {
            path += `/${segment}`;

            const label =
                labels.value[segment] ||
                (staticLabels[segment] && t(staticLabels[segment])) ||
                upperFirst(segment.replace(/[-_]/g, ' '));

            return { to: path, label };
        });

        return trail.value?.path === route.path
            ? [...items.slice(0, -1), ...trail.value.items, items.at(-1)!]
            : items;
    });
};
