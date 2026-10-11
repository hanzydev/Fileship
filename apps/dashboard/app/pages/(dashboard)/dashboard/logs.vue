<template>
    <Head>
        <Title>Logs</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-6 p-4 pt-0">
        <PageHeader title="Logs" :description="pluralize(logs.logs.length, 'entry', 'entries')">
            <UiButton
                variant="outline"
                :disabled="!logs.logs.length || isClearing"
                @click="handleClear"
            >
                <UiSpinner v-if="isClearing" />
                <ITrash2 v-else />
                Clear logs
            </UiButton>
        </PageHeader>

        <div class="flex flex-wrap gap-2">
            <UiInputGroup class="max-w-sm min-w-56 flex-1">
                <UiInputGroupAddon>
                    <UiSpinner v-if="isSearching" />
                    <ISearch v-else />
                </UiInputGroupAddon>
                <UiInputGroupInput v-model="search" placeholder="Search logs..." />
            </UiInputGroup>
            <UiSelect v-model="actionFilter">
                <UiSelectTrigger class="w-48">
                    <IListFilter />
                    <UiSelectValue />
                </UiSelectTrigger>
                <UiSelectContent position="popper">
                    <UiSelectItem value="all">All actions</UiSelectItem>
                    <UiSelectSeparator />
                    <UiSelectItem v-for="action in actions" :key="action" :value="action">
                        {{ action }}
                    </UiSelectItem>
                </UiSelectContent>
            </UiSelect>
        </div>

        <div v-if="isLoading" class="flex flex-col gap-2">
            <UiSkeleton v-for="i in 8" :key="i" class="h-12 rounded-2xl" />
        </div>

        <EmptyState
            v-else-if="!filtered.length"
            :icon="IClipboardList"
            :title="logs.logs.length ? 'No matching entries' : 'No logs yet'"
            :description="
                logs.logs.length
                    ? 'Try a different search or filter.'
                    : 'Activity will show up here.'
            "
        />

        <DataTable
            v-else
            v-model:sort="sort"
            :items="sorted"
            :item-key="(l) => l.id"
            :columns="columns"
            :card-estimate-size="88"
        >
            <template #cell-user="{ item }">
                <div class="flex min-w-0 items-center gap-2">
                    <template v-if="getUser(item)">
                        <UiAvatar class="size-6">
                            <UiAvatarImage
                                v-if="getUser(item)!.avatar"
                                :src="getUser(item)!.avatar!"
                            />
                            <UiAvatarFallback>
                                {{ getUser(item)!.username[0]!.toUpperCase() }}
                            </UiAvatarFallback>
                        </UiAvatar>
                        <span class="truncate">{{ getUser(item)!.username }}</span>
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
                <div class="flex flex-col gap-1 rounded-2xl border p-3">
                    <div class="flex items-center gap-2">
                        <UiBadge variant="outline">{{ item.action }}</UiBadge>
                        <TextBlock variant="muted" class="ml-auto text-xs">
                            {{ dayjs(item.createdAt).fromNow() }}
                        </TextBlock>
                    </div>
                    <TextBlock variant="small" class="line-clamp-2">{{ item.message }}</TextBlock>
                    <TextBlock variant="muted" class="text-xs">
                        {{ getUser(item)?.username || 'System' }} · {{ item.ip }}
                    </TextBlock>
                </div>
            </template>
        </DataTable>
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

import { IClipboardList } from '#components';

const logs = useLogs();
const { $toast } = useNuxtApp();

const { status } = useLazyAsyncData('logs', fetchLogs, { server: false });

const search = ref('');
const actionFilter = ref('all');
const isSearching = ref(false);
const isClearing = ref(false);
const searchResults = ref<string[] | null>(null);
const sort = ref({ key: 'createdAt', desc: true });

const isLoading = computed(() => status.value !== 'success' && !logs.value.logs.length);

const columns = [
    { key: 'user', label: 'User', class: 'w-40' },
    { key: 'action', label: 'Action', sortable: true },
    { key: 'message', label: 'Message', class: 'max-w-0 w-full' },
    { key: 'ip', label: 'IP', class: 'hidden xl:table-cell' },
    { key: 'createdAt', label: 'When', sortable: true },
];

const actions = computed(() => [...new Set(logs.value.logs.map((l) => l.action))].sort());

const filtered = computed(() =>
    logs.value.logs.filter(
        (l) =>
            (actionFilter.value === 'all' || l.action === actionFilter.value) &&
            (!searchResults.value || searchResults.value.includes(l.id)),
    ),
);

const sorted = computed(() =>
    [...filtered.value].sort((a, b) => {
        const result =
            sort.value.key === 'action'
                ? a.action.localeCompare(b.action)
                : +a.createdAt - +b.createdAt;

        return sort.value.desc ? -result : result;
    }),
);

const getUser = (log: LogData) => logs.value.users.find((u) => u.id === log.userId);

const handleClear = async () => {
    const confirmed = await askConfirmation({
        title: 'Clear all logs?',
        description: 'Every log entry will be permanently deleted.',
        confirmText: 'Clear',
        destructive: true,
    });

    if (!confirmed) return;

    isClearing.value = true;

    try {
        const result = await withVerification((verificationData) =>
            $fetch('/api/logs', { method: 'DELETE', body: { verificationData } }),
        );

        if (result) $toast.success('Logs cleared');
    } catch (error: any) {
        $toast.error(error.data.message);
    }

    isClearing.value = false;
};

watchDebounced(
    search,
    async (value) => {
        if (!value.trim()) {
            searchResults.value = null;
            return;
        }

        isSearching.value = true;

        try {
            searchResults.value = await $fetch<string[]>('/api/logs/search', {
                method: 'POST',
                body: { query: value.trim() },
            });
        } catch {
            searchResults.value = [];
        }

        isSearching.value = false;
    },
    { debounce: 400 },
);

definePageMeta({
    layout: 'dashboard',
    middleware: 'admin-only',
});
</script>
