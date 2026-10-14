<template>
    <Head>
        <Title>Instances</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader
            title="Instances"
            :description="
                isLoading
                    ? 'Loading...'
                    : `${pluralize(total, 'instance')}, deleted after ${retentionDays} days without a report`
            "
        />

        <UiInputGroup class="max-w-sm">
            <UiInputGroupAddon>
                <UiSpinner v-if="!!search && isFetching" />
                <ISearch v-else />
            </UiInputGroupAddon>
            <UiInputGroupInput v-model="search" placeholder="Search by ID or version..." />
        </UiInputGroup>

        <EmptyState
            v-if="!isLoading && !instances.length"
            :icon="IServer"
            :title="search ? 'No matching instances' : 'No instances yet'"
            :description="
                search
                    ? 'Try a different search.'
                    : 'Instances appear here after their first report.'
            "
        />

        <DataTable
            v-else
            v-model:sort="sort"
            :items="instances"
            :item-key="(instance) => instance.id"
            :columns="columns"
            :card-estimate-size="74"
            :loading="isLoading"
            :loading-more="isLoadingMore"
            @row-click="handleOpen"
            @reach-end="handleLoadMore"
        >
            <template #cell-id="{ item }">
                <span class="block truncate font-mono text-xs">{{ item.id }}</span>
            </template>
            <template #cell-version="{ item }">
                <UiBadge v-if="item.version" variant="secondary">v{{ item.version }}</UiBadge>
            </template>
            <template #cell-users="{ item }">{{ formatNumber(item.users) }}</template>
            <template #cell-files="{ item }">{{ formatNumber(item.files) }}</template>
            <template #cell-storage="{ item }">
                <span class="block truncate">
                    {{ item.storage ? formatBytes(item.storage) : '—' }}
                </span>
            </template>
            <template #cell-platform="{ item }">
                <span class="flex min-w-0 items-center gap-1.5 text-muted-foreground">
                    <span class="truncate">
                        {{ [item.platform, item.arch].filter(Boolean).join(' · ') || '—' }}
                    </span>
                    <IContainer v-if="item.docker" class="size-3.5 shrink-0" />
                </span>
            </template>
            <template #cell-lastSeenAt="{ item }">
                <span class="block truncate text-muted-foreground">
                    {{ dayjs(item.lastSeenAt).fromNow() }}
                </span>
            </template>
            <template #cell-firstSeenAt="{ item }">
                <span class="block truncate text-muted-foreground">
                    {{ dayjs(item.firstSeenAt).fromNow() }}
                </span>
            </template>
            <template #card-skeleton>
                <ListCardSkeleton media="size-4" />
            </template>
            <template #card="{ item }">
                <NuxtLink :to="`/instances/${item.id}`">
                    <UiCard size="sm">
                        <UiCardContent class="flex min-h-8 items-center gap-3">
                            <IServer class="size-4 shrink-0 text-muted-foreground" />
                            <div class="flex min-w-0 flex-1 flex-col">
                                <TextBlock variant="small" class="truncate font-mono">
                                    {{ item.id.slice(0, 8) }}
                                </TextBlock>
                                <TextBlock variant="muted" class="truncate text-xs">
                                    {{ item.version ? `v${item.version}` : 'Unknown version' }} ·
                                    {{ pluralize(item.users, 'user') }} ·
                                    {{ dayjs(item.lastSeenAt).fromNow() }}
                                </TextBlock>
                            </div>
                            <IChevronRight class="size-4 shrink-0 text-muted-foreground" />
                        </UiCardContent>
                    </UiCard>
                </NuxtLink>
            </template>
        </DataTable>
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

import { IServer } from '#components';

const PAGE_SIZE = 50;

const { $toast } = useNuxtApp();

const search = useQueryState('q', '');
const sort = ref({ key: 'lastSeenAt', desc: true });

const fetchPage = (offset: number) =>
    $fetch('/api/instances', {
        query: {
            q: search.value.trim(),
            sort: `${sort.value.key}:${sort.value.desc ? 'desc' : 'asc'}`,
            offset,
            limit: PAGE_SIZE,
        },
    });

type InstancePage = Awaited<ReturnType<typeof fetchPage>>;

const instances = ref<InstancePage['items']>([]);
const total = ref(0);
const retentionDays = ref(90);
const isLoading = ref(true);
const isFetching = ref(false);
const isLoadingMore = ref(false);

let version = 0;

const columns = [
    { key: 'id', label: 'Instance' },
    { key: 'version', label: 'Version', sortable: true, descFirst: true, class: 'w-28' },
    { key: 'users', label: 'Users', sortable: true, descFirst: true, class: 'w-20' },
    { key: 'files', label: 'Files', sortable: true, descFirst: true, class: 'w-24' },
    {
        key: 'storage',
        label: 'Storage',
        sortable: true,
        descFirst: true,
        class: 'hidden w-28 lg:table-cell',
    },
    { key: 'platform', label: 'Platform', class: 'hidden w-36 xl:table-cell' },
    { key: 'lastSeenAt', label: 'Last seen', sortable: true, descFirst: true, class: 'w-32' },
    {
        key: 'firstSeenAt',
        label: 'First seen',
        sortable: true,
        descFirst: true,
        class: 'hidden w-32 xl:table-cell',
    },
];

const load = async () => {
    const current = ++version;
    isFetching.value = true;

    try {
        const page = await fetchPage(0);

        if (current === version) {
            instances.value = page.items;
            total.value = page.total;
            retentionDays.value = page.retentionDays;
        }
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    if (current === version) {
        isLoading.value = false;
        isFetching.value = false;
    }
};

const handleLoadMore = async () => {
    if (isFetching.value || isLoadingMore.value || instances.value.length >= total.value) return;

    const current = version;
    isLoadingMore.value = true;

    const page = await fetchPage(instances.value.length).catch(() => null);

    if (page && current === version) {
        const known = new Set(instances.value.map((instance) => instance.id));
        instances.value.push(...page.items.filter((instance) => !known.has(instance.id)));
        total.value = page.total;
    }

    isLoadingMore.value = false;
};

const handleOpen = (instance: InstancePage['items'][number]) =>
    navigateTo(`/instances/${instance.id}`);

watchDebounced(search, load, { debounce: 250 });
watch(sort, load, { deep: true });

onMounted(load);

definePageMeta({
    layout: 'panel',
});
</script>
