<template>
    <div>
        <Head>
            <Title>Logs | Admin Panel</Title>
        </Head>

        <div space-y-6>
            <h2>Logs</h2>
            <UiSearchBar v-model="searchQuery" placeholder="Search logs..." />
            <UiTable
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
                                    user?.avatar
                                        ? h(UiAvatar, {
                                              size: 'xs',
                                              src: user.avatar,
                                              alt: user.username,
                                              class: '!ring-0',
                                          })
                                        : h(Icon, {
                                              name: system
                                                  ? 'heroicons-solid:chip'
                                                  : 'iconamoon:profile-fill',
                                              size: '20',
                                          }),
                                    h(
                                        'p',
                                        {
                                            class: 'text-slate200',
                                        },
                                        system
                                            ? 'System'
                                            : user?.username || 'Deleted User',
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
                        resolve: ({ createdAt }) => moment(createdAt).fromNow(),
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
import moment from 'moment';

import { Icon, UiAvatar } from '#components';

const logs = useLogs();

const searchQuery = ref('');
const currentPage = ref(1);

const { data } = await useFetch('/api/logs');

logs.value = data.value!.map((l) => ({
    ...l,
    createdAt: new Date(l.createdAt),
}));

const filtered = computed(() =>
    logs.value.filter((l) =>
        Object.values(l).some((v) =>
            v
                ?.toString()
                ?.toLowerCase()
                ?.includes(searchQuery.value.toLowerCase()),
        ),
    ),
);

const calculatedLogs = computed(() => {
    const start = (currentPage.value - 1) * 20;
    const end = start + 20;
    return filtered.value.slice(start, end);
});

definePageMeta({
    layout: 'admin',
    middleware: 'admin-only',
});
</script>
