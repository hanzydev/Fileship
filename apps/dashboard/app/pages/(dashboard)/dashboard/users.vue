<template>
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

    <ModalsVerifyMFA
        v-model="verifyDeleteModal.open"
        :error="verifyDeleteModal.error"
        :disabled="willBeDeleted.has(verifyDeleteModal.userId)"
        :methods="verifyDeleteModal.methods"
        @got="(data) => handleDelete(verifyDeleteModal.userId, data)"
    />

    <ModalsVerifyMFA
        v-model="verifyActingModal.open"
        :error="verifyActingModal.error"
        :disabled="!!willBeActed"
        :methods="verifyActingModal.methods"
        @got="(data) => handleActAsUser(verifyActingModal.username, data)"
    />

    <ModalsAreYouSure
        v-model="areYouSureDelete.open"
        title="Delete User"
        description="Are you sure you want to delete this user?"
        @confirm="handleDelete(areYouSureDelete.userId)"
    />

    <LazyDashboardContent>
        <template #header>
            <h2>Users</h2>
            <UiButton
                icon="solar:user-plus-bold"
                icon-size="20"
                alignment="center"
                variant="accent"
                gap-2
                @click="createUserModalOpen = true"
            >
                Create User
            </UiButton>
        </template>
        <UiSearchBar
            v-model="searchQuery"
            v-model:loading="isSearching"
            placeholder="Search users..."
        />
        <UiTable
            ring="1 fs-overlay-4"
            :loading="isLoading"
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
                                h(UiAvatar, {
                                    size: 'xs',
                                    src: user.avatar,
                                    alt: user.username,
                                }),
                                h(
                                    'p',
                                    {
                                        class: 'text-fs-muted-1',
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
                            name: superAdmin ? 'lucide:check' : 'lucide:x',
                            size: '20',
                        }),
                },
                {
                    key: 'permissions',
                    width: '25%',
                    resolve: ({ permissions }) => permissions.join(', '),
                },
                {
                    key: 'files',
                    width: '5%',
                    resolve: ({ stats }) => Intl.NumberFormat().format(stats.files),
                },
                {
                    key: 'notes',
                    width: '5%',
                    resolve: ({ stats }) => Intl.NumberFormat().format(stats.notes),
                },
                {
                    key: 'createdAt',
                    width: '15%',
                    resolve: ({ createdAt }) => dayjs(createdAt).fromNow(),
                },
                {
                    key: 'Quick Actions',
                    width: '20%',
                    render: (row) => {
                        return h('div', { class: 'flex items-center gap4' }, [
                            h(UiButton, {
                                variant: 'outline',
                                alignment: 'center',
                                class: 'h8 w8 !p0 text-fs-muted-2 hover:text-white',
                                icon: 'lucide:arrow-right-left',
                                iconSize: '20',
                                disabled:
                                    (row.superAdmin && !currentUser!.superAdmin) ||
                                    row.user.username === currentUser!.username ||
                                    willBeActed,
                                loading: willBeActed === row.user.username,
                                'aria-label': 'Act as user',
                                onClick: () => handleActAsUser(row.user.username),
                            }),
                            h(UiButton, {
                                variant: 'outline',
                                alignment: 'center',
                                class: 'h8 w8 !p0 text-fs-muted-2 hover:text-white',
                                icon: 'solar:pen-2-bold',
                                iconSize: '20',
                                disabled: row.superAdmin && !currentUser!.superAdmin,
                                'aria-label': 'Edit user',
                                onClick: () => {
                                    editModal.user = {
                                        ...row.user,
                                        ...row,
                                    };
                                    nextTick(() => (editModal.open = true));
                                },
                            }),
                            h(UiButton, {
                                variant: 'outline',
                                alignment: 'center',
                                class: 'h8 w8 !p0 ring-red-500 text-fs-muted-2 hover:text-white hover:!bg-red-500',
                                icon: 'solar:trash-bin-minimalistic-bold',
                                iconSize: '20',
                                disabled:
                                    (row.superAdmin && !currentUser!.superAdmin) ||
                                    row.user.username === currentUser!.username ||
                                    willBeDeleted.has(row.user.id) ||
                                    willBeActed === row.user.username,
                                loading: willBeDeleted.has(row.user.id),
                                'aria-label': 'Delete user',
                                onClick: () => {
                                    areYouSureDelete.userId = row.user.id;
                                    areYouSureDelete.open = true;
                                },
                            }),
                        ]);
                    },
                },
            ]"
            :rows="
                calculatedUsers.map(({ id, username, avatar, ...u }) => ({
                    ...u,
                    user: { id, username, avatar },
                }))
            "
        />
        <UiPagination v-model="currentPage" :item-count="filtered.length" :items-per-page="20" />
    </LazyDashboardContent>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

import { Icon, UiAvatar, UiButton } from '#components';

const currentUser = useAuthUser();
const currentTheme = useTheme();
const users = useUsers();
const { $toast } = useNuxtApp();

const currentPage = ref(1);
const searchQuery = ref('');
const searched = ref<string[]>([]);

const isSearching = ref(false);
const isLoading = ref(!users.value.length);

const willBeDeleted = ref(new Set<string>());
const willBeActed = ref<string | null>(null);

const createUserModalOpen = ref(false);

const editModal = reactive({
    user: null as UserData | null,
    open: false,
});

const verifyDeleteModal = reactive({
    userId: '',
    open: false,
    error: undefined as string | undefined,
    methods: [],
});

const verifyActingModal = reactive({
    username: '',
    open: false,
    error: undefined as string | undefined,
    methods: [],
});

const areYouSureDelete = reactive({
    open: false,
    userId: '',
});

const handleActAsUser = async (username: string, verificationData?: any) => {
    try {
        willBeActed.value = username;
        verifyActingModal.error = undefined;

        useCookie('adminSessionId', {
            expires: new Date(Date.now() + 1000 * 60 * 60 * 6),
            path: '/',
            sameSite: 'lax',
        }).value = useCookie('sessionId').value;

        const { user, session } = await $fetch('/api/auth/login', {
            method: 'POST',
            body: {
                username,
                password: '--------',
                verificationData,
            },
        });

        currentUser.value = {
            ...user,
            currentSessionId: session.id,
            createdAt: new Date(user.createdAt),
        };

        currentTheme.value = user.theme as never;

        clearStates(true);

        await nextTick();
        await refreshNuxtData();
        await navigateTo('/dashboard');

        initSocket();
    } catch (error: any) {
        if (verifyActingModal.open) {
            verifyActingModal.error = error.data.message;
        } else {
            verifyActingModal.open = true;
            verifyActingModal.username = username;
            verifyActingModal.methods = error.data.data.mfa.methods;
        }

        useCookie('adminSessionId').value = null;
    }

    willBeActed.value = null;
};

const handleDelete = async (id: string, verificationData?: any) => {
    willBeDeleted.value.add(id);
    verifyDeleteModal.error = undefined;

    try {
        await $fetch(`/api/users/${id}`, {
            method: 'DELETE',
            body: {
                verificationData,
            },
        });
        $toast.success('User deleted successfully');

        verifyDeleteModal.open = false;
    } catch (error: any) {
        if (verifyDeleteModal.open) {
            verifyDeleteModal.error = error.data.message;
        } else if (error.data.message === 'Verification is required') {
            verifyDeleteModal.open = true;
            verifyDeleteModal.userId = id;
            verifyDeleteModal.methods = error.data.data.mfa.methods;
        } else if (!verifyDeleteModal.open) {
            $toast.error(error.data.message);
        }
    }

    willBeDeleted.value.delete(id);
};

const filtered = computed(() =>
    users.value.filter((u) =>
        !isSearching.value && searchQuery.value.length ? searched.value.includes(u.id) : true,
    ),
);

const calculatedUsers = computed(() => {
    const start = (currentPage.value - 1) * 20;
    const end = start + 20;
    return filtered.value.slice(start, end);
});

onMounted(async () => {
    if (!users.value.length) {
        const data = await $fetch('/api/users');

        users.value = data.map((u) => ({
            ...u,
            createdAt: new Date(u.createdAt),
        }));

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
                searched.value = await $fetch<string[]>('/api/users/search', {
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
    layout: 'dashboard',
    middleware: 'admin-only',
});
</script>
