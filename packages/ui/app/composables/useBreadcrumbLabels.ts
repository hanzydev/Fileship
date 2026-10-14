export const useBreadcrumbLabels = () => {
    return useState<Record<string, string>>('breadcrumbLabels', () => ({}));
};
