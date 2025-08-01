<template>
    <div>
        <Head>
            <Title>Logs | Admin Panel</Title>
        </Head>

        <ModalsAreYouSure
            v-model="areYouSureModalOpen"
            title="Are you really sure you want to flush all logs?"
            description="All logs will be deleted and cannot be undone."
            :disabled="isFlushingLogs"
            @confirm="handleFlushLogs"
        />

        <div space-y-6>
            <div flex="~ items-center justify-between">
                <h2>Logs</h2>
                <UiButton
                    v-if="currentUser?.superAdmin"
                    icon="heroicons-solid:trash"
                    icon-size="20"
                    alignment="center"
                    variant="dangerFill"
                    class="h8 w8 !p0"
                    aria-label="Flush logs"
                    :disabled="
                        isFlushingLogs || !logs.logs.filter((l) => l.action !== 'Flush Logs').length
                    "
                    :loading="isFlushingLogs"
                    @click="areYouSureModalOpen = true"
                />
            </div>
            <UiSearchBar
                v-model="searchQuery"
                v-model:loading="isSearching"
                placeholder="Search logs..."
            />
            <UiTable
                ring="1 fs-overlay-4"
                :loading="isLoading"
                :columns="[
                    {
                        key: 'ip',
                        width: '10%',
                    },
                    {
                        key: 'action',
                        width: '10%',
                    },
                    {
                        key: 'user',
                        width: '15%',
                        render: ({ user, system }) =>
                            h(
                                'div',
                                {
                                    class: 'flex items-center gap2',
                                },
                                [
                                    h(UiAvatar, {
                                        size: 'xs',
                                        src: user?.avatar,
                                        alt: system
                                            ? 'System'
                                            : upperFirst(user?.username || 'Deleted User'),
                                    }),
                                    h(
                                        'p',
                                        {
                                            class: 'text-fs-muted-1',
                                        },
                                        system
                                            ? 'System'
                                            : upperFirst(user?.username || 'Deleted User'),
                                    ),
                                ],
                            ),
                    },
                    {
                        key: 'message',
                        width: '35%',
                    },
                    {
                        key: 'createdAt',
                        width: '20%',
                        resolve: ({ createdAt }) => dayjs(createdAt).fromNow(),
                    },
                ]"
                :rows="calculatedLogs"
                nothing-here-message="There are no logs to display."
                nothing-here-icon="heroicons-solid:document-search"
            />
            <UiPagination
                v-model="currentPage"
                :item-count="filtered.length"
                :items-per-page="20"
            />
        </div>
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';
import { upperFirst } from 'scule';

import { UiAvatar } from '#components';

const logs = useLogs();
const currentUser = useAuthUser();
const { $toast } = useNuxtApp();

const currentPage = ref(1);
const searchQuery = ref('');
const searched = ref<string[]>([]);

const areYouSureModalOpen = ref(false);

const isSearching = ref(false);
const isLoading = ref(!logs.value.logs.length);
const isFlushingLogs = ref(false);

const filtered = computed(() =>
    logs.value.logs.filter((l) =>
        !isSearching.value && searchQuery.value.length ? searched.value.includes(l.id) : true,
    ),
);

const calculatedLogs = computed<(LogData & { user: LogUser | null })[]>(() => {
    const start = (currentPage.value - 1) * 20;
    const end = start + 20;
    return filtered.value
        .map((l) => ({
            ...l,
            user: logs.value.users.find((u) => u.id === l.userId) || null,
        }))
        .slice(start, end);
});

const handleFlushLogs = async () => {
    isFlushingLogs.value = true;

    await $fetch('/api/logs', {
        method: 'DELETE',
    });

    isFlushingLogs.value = false;

    $toast.success('All logs flushed successfully');
};

onMounted(async () => {
    if (!logs.value.logs.length) {
        const data = await $fetch('/api/logs');

        logs.value = {
            users: data.users,
            logs: data.logs.map((l) => ({
                ...l,
                createdAt: new Date(l.createdAt),
            })),
        };

        isLoading.value = false;
    }
});

let searchTimeout: NodeJS.Timeout;

watch(searchQuery, (query) => {
    clearTimeout(searchTimeout);

    if (query.length) {
        isSearching.value = true;

        searchTimeout = setTimeout(async () => {
            try {
                searched.value = await $fetch<string[]>('/api/logs/search', {
                    method: 'POST',
                    body: { query },
                });
            } catch {
                searched.value = [];
            }

            isSearching.value = false;
        }, 750);
    } else {
        searched.value = [];
    }
});

definePageMeta({
    layout: 'admin',
    middleware: 'admin-only',
});
</script>
