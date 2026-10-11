let pendingQuery: Record<string, string | undefined> | null = null;

export const useQueryState = <T extends string = string>(key: string, defaultValue: NoInfer<T>) => {
    const route = useRoute();
    const router = useRouter();

    return computed<T>({
        get: () => (route.query[key] as T) || defaultValue,
        set: (value) => {
            const isFirst = !pendingQuery;

            pendingQuery = {
                ...pendingQuery,
                [key]: value && value !== defaultValue ? value : undefined,
            };

            if (!isFirst) return;

            nextTick(() => {
                const query = { ...route.query, ...pendingQuery };
                pendingQuery = null;
                router.replace({ query });
            });
        },
    });
};
