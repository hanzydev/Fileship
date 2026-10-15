<template>
    <Head>
        <Title>Logs</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader
            title="Logs"
            :description="isLoading ? 'Loading...' : pluralize(total, 'entry', 'entries')"
        >
            <UiButton
                v-if="currentUser!.superAdmin"
                variant="outline"
                :disabled="!logs.length"
                @click="handleClear"
            >
                <ITrash2 />
                Clear logs
            </UiButton>
        </PageHeader>

        <div class="flex flex-wrap gap-2">
            <UiInputGroup class="max-w-sm flex-1">
                <UiInputGroupAddon>
                    <UiSpinner v-if="!!search && isFetching" />
                    <ISearch v-else />
                </UiInputGroupAddon>
                <UiInputGroupInput v-model="search" placeholder="Search logs..." />
            </UiInputGroup>
            <UiDropdownMenu @update:open="$event && refreshActions()">
                <UiDropdownMenuTrigger as-child>
                    <UiButton variant="outline" class="max-w-52">
                        <IListFilter />
                        <span class="truncate">
                            {{ actionFilter === 'all' ? 'All actions' : actionFilter }}
                        </span>
                        <IChevronDown class="opacity-50" />
                    </UiButton>
                </UiDropdownMenuTrigger>
                <UiDropdownMenuContent align="start" class="w-56">
                    <UiDropdownMenuLabel>Action</UiDropdownMenuLabel>
                    <UiDropdownMenuRadioGroup v-model="actionFilter">
                        <UiDropdownMenuRadioItem value="all">All actions</UiDropdownMenuRadioItem>
                        <UiDropdownMenuSeparator />
                        <UiDropdownMenuRadioItem
                            v-for="action in actions"
                            :key="action"
                            :value="action"
                        >
                            {{ action }}
                        </UiDropdownMenuRadioItem>
                    </UiDropdownMenuRadioGroup>
                </UiDropdownMenuContent>
            </UiDropdownMenu>
            <UiDropdownMenu>
                <UiDropdownMenuTrigger as-child>
                    <UiButton variant="outline" class="md:hidden">
                        <IArrowDownUp />
                        {{ sortOptions.find((option) => option.value === sortValue)?.label }}
                        <IChevronDown class="opacity-50" />
                    </UiButton>
                </UiDropdownMenuTrigger>
                <UiDropdownMenuContent align="end" class="w-56">
                    <UiDropdownMenuLabel>Sort by</UiDropdownMenuLabel>
                    <UiDropdownMenuRadioGroup v-model="sortValue">
                        <UiDropdownMenuRadioItem
                            v-for="option in sortOptions"
                            :key="option.value"
                            :value="option.value"
                        >
                            {{ option.label }}
                        </UiDropdownMenuRadioItem>
                    </UiDropdownMenuRadioGroup>
                </UiDropdownMenuContent>
            </UiDropdownMenu>
        </div>

        <EmptyState
            v-if="isError"
            :icon="ICircleAlert"
            title="Couldn't load logs"
            description="Something went wrong. Check your connection and try again."
        >
            <UiButton variant="outline" @click="refresh">Try again</UiButton>
        </EmptyState>

        <EmptyState
            v-else-if="!isLoading && !logs.length"
            :icon="IClipboardList"
            :title="isFiltered ? 'No matching entries' : 'No logs yet'"
            :description="
                isFiltered ? 'Try a different search or filter.' : 'Activity will show up here.'
            "
        >
            <UiButton v-if="isFiltered" variant="outline" @click="handleClearFilters">
                Clear filters
            </UiButton>
        </EmptyState>

        <DataTable
            v-else
            v-model:sort="sort"
            :items="logs"
            :item-key="(l) => l.id"
            :columns="columns"
            :card-estimate-size="88"
            :loading="isLoading"
            :loading-more="isLoadingMore"
            @reach-end="loadMore"
        >
            <template #cell-user="{ item }">
                <div class="flex min-w-0 items-center gap-2">
                    <template v-if="item.user">
                        <UiAvatar class="size-6">
                            <UiAvatarImage v-if="item.user!.avatar" :src="item.user!.avatar!" />
                            <UiAvatarFallback>
                                {{ item.user!.username.slice(0, 2).toUpperCase() }}
                            </UiAvatarFallback>
                        </UiAvatar>
                        <span class="truncate">{{ item.user!.username }}</span>
                    </template>
                    <UiBadge v-else variant="secondary">
                        {{ item.system ? 'System' : 'Unknown' }}
                    </UiBadge>
                </div>
            </template>
            <template #cell-action="{ item }">
                <UiBadge variant="outline" class="max-w-full">
                    <span class="truncate">{{ item.action }}</span>
                </UiBadge>
            </template>
            <template #cell-message="{ item }">
                <span class="block truncate" :title="item.message">{{ item.message }}</span>
            </template>
            <template #cell-ip="{ item }">
                <span class="block truncate font-mono text-muted-foreground">{{ item.ip }}</span>
            </template>
            <template #cell-createdAt="{ item }">
                <UiTooltip>
                    <UiTooltipTrigger as-child>
                        <span class="block truncate text-muted-foreground">
                            {{ dayjs(item.createdAt).fromNow() }}
                        </span>
                    </UiTooltipTrigger>
                    <UiTooltipContent>
                        {{ dayjs(item.createdAt).format('MMM D, YYYY HH:mm:ss') }}
                    </UiTooltipContent>
                </UiTooltip>
            </template>
            <template #card-skeleton>
                <UiCard size="sm">
                    <UiCardContent class="flex flex-col gap-1">
                        <div class="flex items-center gap-2">
                            <UiSkeleton class="h-5 w-28 rounded-full" />
                            <TextBlock
                                custom-tag="div"
                                variant="muted"
                                class="ml-auto w-16 text-xs"
                            >
                                <UiSkeleton class="h-lh w-full rounded-full" />
                            </TextBlock>
                        </div>
                        <TextBlock custom-tag="div" variant="small" class="w-full">
                            <UiSkeleton class="h-lh w-3/4 rounded-full" />
                        </TextBlock>
                        <TextBlock custom-tag="div" variant="muted" class="w-full text-xs">
                            <UiSkeleton class="h-lh w-1/3 rounded-full" />
                        </TextBlock>
                    </UiCardContent>
                </UiCard>
            </template>
            <template #card="{ item }">
                <UiCard size="sm">
                    <UiCardContent class="flex flex-col gap-1">
                        <div class="flex items-center gap-2">
                            <UiBadge variant="outline">{{ item.action }}</UiBadge>
                            <TextBlock variant="muted" class="ml-auto text-xs">
                                {{ dayjs(item.createdAt).fromNow() }}
                            </TextBlock>
                        </div>
                        <TextBlock variant="small" class="line-clamp-2">
                            {{ item.message }}
                        </TextBlock>
                        <TextBlock variant="muted" class="text-xs">
                            {{ item.user?.username || 'System' }} · {{ item.ip }}
                        </TextBlock>
                    </UiCardContent>
                </UiCard>
            </template>
        </DataTable>
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

import { ICircleAlert, IClipboardList } from '#components';

import type { LogData } from '~/types/logs';

const { $toast } = useNuxtApp();
const currentUser = useAuthUser();

const search = ref('');
const actionFilter = ref('all');
const sort = ref({ key: 'createdAt', desc: true });

const columns = [
    { key: 'user', label: 'User', sortable: true, class: 'w-40' },
    { key: 'action', label: 'Action', sortable: true, class: 'hidden w-48 lg:table-cell' },
    { key: 'message', label: 'Message' },
    { key: 'ip', label: 'IP', class: 'hidden w-40 xl:table-cell' },
    { key: 'createdAt', label: 'When', sortable: true, descFirst: true, class: 'w-32' },
];

const sortOptions = [
    { label: 'Newest', value: 'createdAt:desc' },
    { label: 'Oldest', value: 'createdAt:asc' },
    { label: 'Action A–Z', value: 'action:asc' },
    { label: 'User A–Z', value: 'user:asc' },
];

const sortValue = computed({
    get: () => `${sort.value.key}:${sort.value.desc ? 'desc' : 'asc'}`,
    set: (value) => {
        const [key, dir] = value.split(':');
        sort.value = { key: key!, desc: dir === 'desc' };
    },
});

const handleClearFilters = () => {
    search.value = '';
    actionFilter.value = 'all';
};

const isFiltered = computed(() => !!search.value.trim() || actionFilter.value !== 'all');

const {
    items: logs,
    total,
    isLoading,
    isError,
    isFetching,
    isLoadingMore,
    loadMore,
    refresh,
} = usePaginatedList<LogData>('logs', '/api/logs', () => ({
    q: search.value.trim(),
    action: actionFilter.value === 'all' ? undefined : actionFilter.value,
    sort: `${sort.value.key}:${sort.value.desc ? 'desc' : 'asc'}`,
}));

const { data: actions, refresh: refreshActions } = useLazyAsyncData(
    'log-actions',
    () => $fetch('/api/logs/actions'),
    { server: false, default: () => [] },
);

const handleClear = async () => {
    await askConfirmation({
        title: 'Clear all logs?',
        description: 'Every log entry will be permanently deleted.',
        confirmText: 'Clear',
        destructive: true,
        action: async () => {
            try {
                const result = await withVerification((verificationData) =>
                    $fetch('/api/logs', { method: 'DELETE', body: { verificationData } }),
                );

                if (result) $toast.success('Logs cleared');
            } catch (error: any) {
                $toast.error(getErrorMessage(error));
            }
        },
    });
};

definePageMeta({
    layout: 'dashboard',
    middleware: 'admin-only',
});
</script>
