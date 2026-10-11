export const useQueryState = <T extends string>(key: string, defaultValue: T) => {
    const route = useRoute();
    const router = useRouter();

    return computed<T>({
        get: () => (route.query[key] as T) || defaultValue,
        set: (value) =>
            router.replace({
                query: { ...route.query, [key]: value === defaultValue ? undefined : value },
            }),
    });
};
