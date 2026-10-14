import { upperFirst } from 'scule';

export const useBreadcrumbs = (staticLabels: Record<string, string>) => {
    const route = useRoute();
    const labels = useBreadcrumbLabels();

    return computed(() => {
        const segments = route.path.split('/').filter(Boolean);
        let path = '';

        return segments.map((segment) => {
            path += `/${segment}`;

            const label =
                labels.value[segment] ||
                staticLabels[segment] ||
                upperFirst(segment.replace(/[-_]/g, ' '));

            return { to: path, label };
        });
    });
};
