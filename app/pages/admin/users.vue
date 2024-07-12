<template>
    <div>
        <Head>
            <Title>Users</Title>
        </Head>

        <ModalsCreateUser v-model="createUserModalOpen" />
        <ModalsEditUser
            v-if="editModal.user"
            :key="editModal.user.id"
            v-model="editModal.open"
            :data="editModal.user!"
        />

        <div space-y-6>
            <div flex="~ items-center justify-between">
                <h2>Users</h2>
                <UiButton
                    icon="heroicons-solid:user-add"
                    icon-size="20"
                    alignment="center"
                    variant="accent"
                    class="h-8 w-8 !p-0"
                    aria-label="Take notes"
                    @click="createUserModalOpen = true"
                />
            </div>

            <div space-y-4>
                <UiSearchBar
                    v-model="searchQuery"
                    placeholder="Search users..."
                />
                <UiTable
                    :columns="[
                        {
                            key: 'user',
                            width: '15%',
                            render: ({ user }) =>
                                h(
                                    'div',
                                    {
                                        class: 'flex items-center gap2',
                                    },
                                    [
                                        user.avatar
                                            ? h(UiAvatar, {
                                                  size: 'xs',
                                                  src: user.avatar,
                                                  alt: user.username,
                                                  class: '!ring-0',
                                              })
                                            : h(Icon, {
                                                  name: 'iconamoon:profile-fill',
                                                  size: '20',
                                              }),
                                        h(
                                            'p',
                                            {
                                                class: 'text-slate200',
                                            },
                                            user.username,
                                        ),
                                    ],
                                ),
                        },
                        {
                            key: 'superAdmin',
                            width: '10%',
                            render: ({ superAdmin }) =>
                                h(Icon, {
                                    name: superAdmin
                                        ? 'heroicons-solid:check'
                                        : 'heroicons-solid:x',
                                    size: '20',
                                }),
                        },
                        {
                            key: 'permissions',
                            width: '25%',
                            resolve: ({ permissions }) =>
                                permissions.join(', '),
                        },
                        {
                            key: 'files',
                            width: '5%',
                            resolve: ({ _count }) =>
                                Intl.NumberFormat().format(_count?.files ?? 0),
                        },
                        {
                            key: 'notes',
                            width: '5%',
                            resolve: ({ _count }) =>
                                Intl.NumberFormat().format(_count?.notes ?? 0),
                        },
                        {
                            key: 'codes',
                            width: '5%',
                            resolve: ({ _count }) =>
                                Intl.NumberFormat().format(_count?.codes ?? 0),
                        },
                        {
                            key: 'urls',
                            width: '5%',
                            resolve: ({ _count }) =>
                                Intl.NumberFormat().format(_count?.urls ?? 0),
                        },
                        {
                            key: 'createdAt',
                            width: '15%',
                            resolve: ({ createdAt }) =>
                                dayjs(createdAt).fromNow(),
                        },
                        {
                            key: 'Actions',
                            width: '20%',
                            render: (row) => {
                                return h(
                                    'div',
                                    { class: 'flex items-center gap4' },
                                    [
                                        h(UiButton, {
                                            variant: 'outline',
                                            alignment: 'center',
                                            class: 'h8 w8 !p0 text-slate300 hover:text-white',
                                            icon: 'heroicons-solid:switch-horizontal',
                                            iconSize: '20',
                                            disabled:
                                                (row.superAdmin &&
                                                    !currentUser!.superAdmin) ||
                                                row.user.username ===
                                                    currentUser!.username ||
                                                switching,
                                            loading: switching,
                                            onClick: () =>
                                                handleSwitch(row.user.username),
                                        }),
                                        h(UiButton, {
                                            variant: 'outline',
                                            alignment: 'center',
                                            class: 'h8 w8 !p0 text-slate300 hover:text-white',
                                            icon: 'heroicons:pencil-16-solid',
                                            iconSize: '20',
                                            disabled:
                                                row.superAdmin &&
                                                !currentUser!.superAdmin,
                                            onClick: () => {
                                                editModal.user = {
                                                    ...row.user,
                                                    ...row,
                                                };
                                                nextTick(
                                                    () =>
                                                        (editModal.open = true),
                                                );
                                            },
                                        }),
                                        h(UiButton, {
                                            variant: 'outline',
                                            alignment: 'center',
                                            class: 'h8 w8 !p0 ring-red-500 text-slate300 hover:text-white hover:!bg-red-500',
                                            icon: 'heroicons-solid:trash',
                                            iconSize: '20',
                                            disabled:
                                                (row.superAdmin &&
                                                    !currentUser!.superAdmin) ||
                                                row.user.username ===
                                                    currentUser!.username ||
                                                willBeDeleted.has(
                                                    row.user.id,
                                                ) ||
                                                switching,
                                            loading: willBeDeleted.has(
                                                row.user.id,
                                            ),
                                            onClick: () =>
                                                handleDelete(row.user.id),
                                        }),
                                    ],
                                );
                            },
                        },
                    ]"
                    :rows="
                        calculatedUsers.map(
                            ({ id, username, avatar, ...u }) => ({
                                ...u,
                                user: { id, username, avatar },
                            }),
                        )
                    "
                />
                <UiPagination
                    v-model="currentPage"
                    :item-count="calculatedUsers.length"
                    :items-per-page="20"
                />
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';
import { toast } from 'vue-sonner';

import { Icon, UiAvatar, UiButton } from '#components';

const currentUser = useAuthUser();
const users = useUsers();

const searchQuery = ref('');
const currentPage = ref(1);

const willBeDeleted = ref(new Set<string>());

const createUserModalOpen = ref(false);
const switching = ref(false);

const editModal = reactive({
    user: null as UserData | null,
    open: false,
});

const handleSwitch = async (username: string) => {
    switching.value = true;

    useCookie('adminSessionId', {
        expires: new Date(Date.now() + 1000 * 60 * 60 * 6),
        path: '/',
        sameSite: true,
    }).value = useCookie('sessionId').value;

    const { user, session } = await $fetch('/api/auth/login', {
        method: 'POST',
        body: {
            username,
            password: '--------',
        },
    });

    currentUser.value = {
        ...user,
        currentSessionId: session.id,
        createdAt: new Date(user.createdAt),
    };

    await nextTick();
    await refreshNuxtData();
    await navigateTo('/dashboard');

    initSocket();

    switching.value = false;
};

const handleDelete = async (id: string) => {
    willBeDeleted.value.add(id);

    try {
        await $fetch(`/api/users/${id}`, {
            method: 'DELETE',
        });
        toast.success('User deleted successfully');
    } catch (error: any) {
        toast.error(error.data.message);
    }

    willBeDeleted.value.delete(id);
};

const { data } = await useFetch('/api/users');

const filtered = computed(() =>
    users.value.filter((l) =>
        Object.values(l).some((v) =>
            v
                ?.toString()
                ?.toLowerCase()
                ?.includes(searchQuery.value.toLowerCase()),
        ),
    ),
);

const calculatedUsers = computed(() => {
    const start = (currentPage.value - 1) * 20;
    const end = start + 20;
    return filtered.value.slice(start, end);
});

users.value = data.value!.map((u) => ({
    ...u,
    createdAt: new Date(u.createdAt),
}));

definePageMeta({
    layout: 'admin',
    middleware: 'admin-only',
});
</script>
