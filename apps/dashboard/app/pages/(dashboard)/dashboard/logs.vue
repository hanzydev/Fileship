<template>
    <Head>
        <Title>Logs</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-6 p-4 pt-0">
        <PageHeader
            title="Logs"
            :description="isLoading ? 'Loading...' : pluralize(total, 'entry', 'entries')"
        >
            <UiButton variant="outline" :disabled="!logs.length" @click="handleClear">
                <ITrash2 />
                Clear logs
            </UiButton>
        </PageHeader>

        <div class="flex flex-wrap gap-2">
            <UiInputGroup class="max-w-sm min-w-56 flex-1">
                <UiInputGroupAddon>
                    <UiSpinner v-if="!!search && isLoading" />
                    <ISearch v-else />
                </UiInputGroupAddon>
                <UiInputGroupInput v-model="search" placeholder="Search logs..." />
            </UiInputGroup>
            <UiSelect v-model="actionFilter" @update:open="$event && refreshActions()">
                <UiSelectTrigger class="w-48">
                    <IListFilter />
                    <UiSelectValue />
                </UiSelectTrigger>
                <UiSelectContent>
                    <UiSelectItem value="all">All actions</UiSelectItem>
                    <UiSelectSeparator />
                    <UiSelectItem v-for="action in actions" :key="action" :value="action">
                        {{ action }}
                    </UiSelectItem>
                </UiSelectContent>
            </UiSelect>
        </div>

        <EmptyState
            v-if="!isLoading && !logs.length"
            :icon="IClipboardList"
            :title="isFiltered ? 'No matching entries' : 'No logs yet'"
            :description="
                isFiltered ? 'Try a different search or filter.' : 'Activity will show up here.'
            "
        />

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
                                {{ item.user!.username[0]!.toUpperCase() }}
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
                <UiBadge variant="outline">{{ item.action }}</UiBadge>
            </template>
            <template #cell-message="{ item }">
                <span class="block truncate" :title="item.message">{{ item.message }}</span>
            </template>
            <template #cell-ip="{ item }">
                <span class="font-mono text-muted-foreground">{{ item.ip }}</span>
            </template>
            <template #cell-createdAt="{ item }">
                <UiTooltip>
                    <UiTooltipTrigger as-child>
                        <span class="whitespace-nowrap text-muted-foreground">
                            {{ dayjs(item.createdAt).fromNow() }}
                        </span>
                    </UiTooltipTrigger>
                    <UiTooltipContent>
                        {{ dayjs(item.createdAt).format('MMM D, YYYY HH:mm:ss') }}
                    </UiTooltipContent>
                </UiTooltip>
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

import { IClipboardList } from '#components';

const { $toast } = useNuxtApp();

const search = ref('');
const actionFilter = ref('all');
const sort = ref({ key: 'createdAt', desc: true });

const columns = [
    { key: 'user', label: 'User', sortable: true, class: 'w-44' },
    { key: 'action', label: 'Action', sortable: true, class: 'w-48' },
    { key: 'message', label: 'Message' },
    { key: 'ip', label: 'IP', class: 'hidden w-40 xl:table-cell' },
    { key: 'createdAt', label: 'When', sortable: true, class: 'w-36' },
];

const isFiltered = computed(() => !!search.value.trim() || actionFilter.value !== 'all');

const {
    items: logs,
    total,
    isLoading,
    isLoadingMore,
    loadMore,
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
                $toast.error(error.data.message);
            }
        },
    });
};

definePageMeta({
    layout: 'dashboard',
    middleware: 'admin-only',
});
</script>
