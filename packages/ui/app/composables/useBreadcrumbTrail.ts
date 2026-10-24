export const useBreadcrumbTrail = () => {
    return useState<{ path: string; items: { to: string; label: string }[] } | null>(
        'breadcrumbTrail',
        () => null,
    );
};
