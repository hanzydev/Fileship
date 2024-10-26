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

        <ModalsVerifyTotp
            v-if="currentUser!.totpEnabled"
            v-model="verifyDeleteModal.open"
            :error="verifyDeleteModal.error"
            :disabled="willBeDeleted.has(verifyDeleteModal.userId)"
            @got="(totp) => handleDelete(verifyDeleteModal.userId, totp)"
        />
        <ModalsVerifyUserPassword
            v-else
            v-model="verifyDeleteModal.open"
            :error="verifyDeleteModal.error"
            :disabled="willBeDeleted.has(verifyDeleteModal.userId)"
            @got="
                (password) => handleDelete(verifyDeleteModal.userId, password)
            "
        />

        <ModalsVerifyTotp
            v-if="currentUser!.totpEnabled"
            v-model="verifyActingModal.open"
            :error="verifyActingModal.error"
            :disabled="!!willBeActed"
            @got="(totp) => handleActAsUser(verifyActingModal.username, totp)"
        />
        <ModalsVerifyUserPassword
            v-else
            v-model="verifyActingModal.open"
            :error="verifyActingModal.error"
            :disabled="!!willBeActed"
            @got="
                (password) =>
                    handleActAsUser(verifyActingModal.username, password)
            "
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
                    aria-label="Create user"
                    @click="createUserModalOpen = true"
                />
            </div>
            <UiSearchBar v-model="searchQuery" placeholder="Search users..." />
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
                                        class: '!ring-0',
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
                        resolve: ({ permissions }) => permissions.join(', '),
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
                        resolve: ({ createdAt }) => dayjs(createdAt).fromNow(),
                    },
                    {
                        key: 'Quick Actions',
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
                                            willBeActed,
                                        loading:
                                            willBeActed === row.user.username,
                                        'aria-label': 'Act as user',
                                        onClick: () =>
                                            handleActAsUser(row.user.username),
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
                                        'aria-label': 'Edit user',
                                        onClick: () => {
                                            editModal.user = {
                                                ...row.user,
                                                ...row,
                                            };
                                            nextTick(
                                                () => (editModal.open = true),
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
                                            willBeDeleted.has(row.user.id) ||
                                            willBeActed === row.user.username,
                                        loading: willBeDeleted.has(row.user.id),
                                        'aria-label': 'Delete user',
                                        onClick: () =>
                                            handleDelete(row.user.id),
                                    }),
                                ],
                            );
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
            <UiPagination
                v-model="currentPage"
                :item-count="calculatedUsers.length"
                :items-per-page="20"
            />
        </div>
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';
import { toast } from 'vue-sonner';

import { useFuse } from '@vueuse/integrations/useFuse';

import { Icon, UiAvatar, UiButton } from '#components';

const currentUser = useAuthUser();
const currentTheme = useTheme();
const users = useUsers();

const searchQuery = ref('');
const currentPage = ref(1);

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
});

const verifyActingModal = reactive({
    username: '',
    open: false,
    error: undefined as string | undefined,
});

const handleActAsUser = async (username: string, verificationData?: string) => {
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
        }

        useCookie('adminSessionId').value = null;
    }

    willBeActed.value = null;
};

const handleDelete = async (id: string, verificationData?: string) => {
    willBeDeleted.value.add(id);
    verifyDeleteModal.error = undefined;

    try {
        await $fetch(`/api/users/${id}`, {
            method: 'DELETE',
            body: {
                verificationData,
            },
        });
        toast.success('User deleted successfully');

        verifyDeleteModal.open = false;
    } catch (error: any) {
        if (verifyDeleteModal.open) {
            verifyDeleteModal.error = error.data.message;
        } else if (error.data.message === 'Verification is required') {
            verifyDeleteModal.open = true;
            verifyDeleteModal.userId = id;
        } else if (!verifyDeleteModal.open) {
            toast.error(error.data.message);
        }
    }

    willBeDeleted.value.delete(id);
};

const { results } = useFuse(searchQuery, users, {
    matchAllWhenSearchEmpty: true,
    fuseOptions: {
        keys: [
            {
                name: 'username',
                weight: 3,
            },
            {
                name: 'permissions',
                weight: 2,
            },
            'domains',
        ],
    },
});

const calculatedUsers = computed<UserData[]>(() => {
    const start = (currentPage.value - 1) * 20;
    const end = start + 20;
    return results.value.map((r) => r.item).slice(start, end);
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

definePageMeta({
    layout: 'admin',
    middleware: 'admin-only',
});
</script>
