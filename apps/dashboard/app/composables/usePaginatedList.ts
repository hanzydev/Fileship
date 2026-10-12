export type ListResource =
    'files' | 'folders' | 'notes' | 'users' | 'logs' | 'sessions' | 'backups' | 'trash';

export type ListHandle = {
    resource: ListResource;
    refresh: () => unknown;
    map: (fn: (item: any) => any) => void;
    remove: (ids: string[]) => void;
};

type ListResponse<T, S> = { items: T[]; total: number; summary?: S };

const PAGE_LIMIT = 200;
const DATE_KEYS = ['createdAt', 'expiresAt', 'lastSeen'];

const handles = new Set<ListHandle>();
const refreshTimers = new Map<ListResource, ReturnType<typeof setTimeout>>();

const forEachHandle = (resource: ListResource, fn: (handle: ListHandle) => unknown) =>
    handles.forEach((handle) => handle.resource === resource && fn(handle));

export const reviveDates = <T>(item: any): T => {
    for (const key of DATE_KEYS) {
        if (typeof item[key] === 'string') item[key] = new Date(item[key]);
    }

    return item;
};

export const refreshLists = (...resources: ListResource[]) =>
    resources.forEach((resource) => {
        clearTimeout(refreshTimers.get(resource));
        refreshTimers.set(
            resource,
            setTimeout(() => forEachHandle(resource, (handle) => handle.refresh()), 300),
        );
    });

export const mapListItems = (resource: ListResource, fn: (item: any) => any) =>
    forEachHandle(resource, (handle) => handle.map(fn));

export const patchListItem = (resource: ListResource, data: { id: string }) =>
    mapListItems(resource, (item) =>
        item.id === data.id ? { ...item, ...reviveDates({ ...data }) } : item,
    );

export const removeListItems = (resource: ListResource, ids: string[]) =>
    forEachHandle(resource, (handle) => handle.remove(ids));

export const useListHandle = (handle: ListHandle) => {
    onMounted(() => handles.add(handle));
    onScopeDispose(() => handles.delete(handle));
};

export const usePaginatedList = <T extends { id: string }, S = undefined>(
    resource: ListResource,
    endpoint: MaybeRefOrGetter<string>,
    params: MaybeRefOrGetter<Record<string, unknown>> = {},
    {
        limit = 50,
        debounce = 250,
        enabled = true,
    }: { limit?: number; debounce?: number; enabled?: MaybeRefOrGetter<boolean> } = {},
) => {
    const items = ref([]) as Ref<T[]>;
    const total = ref(0);
    const summary = ref<S>();
    const status = ref<'pending' | 'success' | 'error'>('pending');
    const isLoadingMore = ref(false);
    const loadedKey = ref<string>();

    let version = 0;

    const query = computed(() =>
        Object.fromEntries(
            Object.entries(toValue(params))
                .filter(
                    ([, value]) =>
                        value !== undefined &&
                        value !== null &&
                        value !== '' &&
                        !(Array.isArray(value) && !value.length),
                )
                .map(([key, value]) => [key, Array.isArray(value) ? value.join(',') : value]),
        ),
    );

    const fetchPage = (offset: number, pageLimit: number) =>
        $fetch<ListResponse<T, S>>(toValue(endpoint), {
            query: { ...query.value, offset, limit: pageLimit },
        });

    const load = async (count = limit) => {
        const current = ++version;
        const key = queryKey.value;

        try {
            const pages = await Promise.all(
                Array.from({ length: Math.ceil(count / PAGE_LIMIT) }, (_, index) =>
                    fetchPage(index * PAGE_LIMIT, Math.min(PAGE_LIMIT, count - index * PAGE_LIMIT)),
                ),
            );

            if (current !== version) return;

            items.value = pages.flatMap((page) => page.items.map(reviveDates<T>));
            total.value = pages[0]!.total;
            if (pages[0]!.summary) summary.value = pages[0]!.summary;
            loadedKey.value = key;
            status.value = 'success';
        } catch {
            if (current === version) status.value = 'error';
        }
    };

    const loadMore = async () => {
        if (
            status.value !== 'success' ||
            isLoadingMore.value ||
            items.value.length >= total.value
        ) {
            return;
        }

        const current = version;
        isLoadingMore.value = true;

        const page = await fetchPage(items.value.length, limit).catch(() => null);

        if (page && current === version) {
            const known = new Set(items.value.map((item) => item.id));

            items.value = [
                ...items.value,
                ...page.items.map(reviveDates<T>).filter((item) => !known.has(item.id)),
            ];
            total.value = page.total;
        }

        isLoadingMore.value = false;
    };

    const refresh = () => toValue(enabled) && load(Math.max(items.value.length, limit));

    useListHandle({
        resource,
        refresh,
        map: (fn) => (items.value = items.value.map(fn)),
        remove: (ids) => {
            const count = items.value.length;
            items.value = items.value.filter((item) => !ids.includes(item.id));
            total.value -= count - items.value.length;
        },
    });

    const queryKey = computed(() => `${toValue(endpoint)}?${JSON.stringify(query.value)}`);

    watchDebounced(
        () => toValue(enabled) && queryKey.value,
        (key) => {
            if (!key) return;

            status.value = 'pending';
            load();
        },
        { debounce },
    );

    onMounted(() => toValue(enabled) && load());

    return {
        items,
        total,
        summary,
        status,
        isLoading: computed(() => status.value === 'pending' && !items.value.length),
        isLoadingMore,
        isFresh: computed(() => loadedKey.value === queryKey.value),
        hasMore: computed(() => items.value.length < total.value),
        query,
        loadMore,
        refresh,
    };
};
