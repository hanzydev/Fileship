export const useQueryState = <T extends string = string>(key: string, defaultValue: NoInfer<T>) => {
    const route = useRoute();
    const router = useRouter();

    return computed<T>({
        get: () => (route.query[key] as T) || defaultValue,
        set: (value) =>
            router.replace({
                query: {
                    ...route.query,
                    [key]: value && value !== defaultValue ? value : undefined,
                },
            }),
    });
};
