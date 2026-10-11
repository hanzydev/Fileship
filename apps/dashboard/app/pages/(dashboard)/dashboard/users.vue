<template>
    <Head>
        <Title>Users</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-6 p-4 pt-0">
        <PageHeader title="Users" :description="pluralize(users.length, 'user')">
            <UiButton @click="openSheet(null)">
                <IUserPlus />
                New user
            </UiButton>
        </PageHeader>

        <UiInputGroup class="max-w-sm">
            <UiInputGroupAddon>
                <UiSpinner v-if="isSearching" />
                <ISearch v-else />
            </UiInputGroupAddon>
            <UiInputGroupInput v-model="search" placeholder="Search users..." />
        </UiInputGroup>

        <div v-if="isLoading" class="flex flex-col gap-2">
            <UiSkeleton v-for="i in 5" :key="i" class="h-12 rounded-2xl" />
        </div>

        <EmptyState
            v-else-if="!filtered.length"
            :icon="ISearchX"
            title="No matching users"
            description="Try a different username."
        />

        <DataTable
            v-else
            v-model:sort="sort"
            :items="sorted"
            :item-key="(u) => u.id"
            :columns="columns"
            :card-estimate-size="72"
        >
            <template #cell-username="{ item }">
                <div class="flex min-w-0 items-center gap-3">
                    <UiAvatar class="size-8">
                        <UiAvatarImage v-if="item.avatar" :src="item.avatar" />
                        <UiAvatarFallback>
                            {{ item.username.slice(0, 2).toUpperCase() }}
                        </UiAvatarFallback>
                    </UiAvatar>
                    <span class="truncate font-medium">{{ item.username }}</span>
                    <UiBadge v-if="item.id === currentUser!.id" variant="secondary">You</UiBadge>
                </div>
            </template>
            <template #cell-role="{ item }">
                <div class="flex flex-wrap gap-1">
                    <UiBadge v-for="role in getRoles(item)" :key="role" variant="outline">
                        {{ role }}
                    </UiBadge>
                </div>
            </template>
            <template #cell-files="{ item }">{{ item.stats.files }}</template>
            <template #cell-notes="{ item }">{{ item.stats.notes }}</template>
            <template #cell-storage="{ item }">
                <span class="text-muted-foreground">
                    {{
                        item.limits.usableSpace === -1
                            ? 'Unlimited'
                            : `${item.limits.usableSpace} MB`
                    }}
                </span>
            </template>
            <template #cell-createdAt="{ item }">
                <span class="text-muted-foreground">
                    {{ dayjs(item.createdAt).format('MMM D, YYYY') }}
                </span>
            </template>
            <template #cell-actions="{ item }">
                <UserMenu
                    :user="item"
                    @edit="openSheet(item)"
                    @impersonate="handleImpersonate(item)"
                    @delete="handleDelete(item)"
                />
            </template>
            <template #card="{ item }">
                <div class="flex items-center gap-3 rounded-2xl border p-3">
                    <UiAvatar>
                        <UiAvatarImage v-if="item.avatar" :src="item.avatar" />
                        <UiAvatarFallback>
                            {{ item.username.slice(0, 2).toUpperCase() }}
                        </UiAvatarFallback>
                    </UiAvatar>
                    <div class="flex min-w-0 flex-1 flex-col">
                        <TextBlock variant="small" class="truncate">{{ item.username }}</TextBlock>
                        <TextBlock variant="muted" class="truncate text-xs">
                            {{ getRoles(item).join(', ') }} ·
                            {{ pluralize(item.stats.files, 'file') }}
                        </TextBlock>
                    </div>
                    <UserMenu
                        :user="item"
                        @edit="openSheet(item)"
                        @impersonate="handleImpersonate(item)"
                        @delete="handleDelete(item)"
                    />
                </div>
            </template>
        </DataTable>

        <UserSheet v-model:open="sheetOpen" :user="editingUser" />
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';
import { titleCase } from 'scule';

import { ISearchX } from '#components';
import { UserPermission } from '#shared/prisma/enums';

const users = useUsers();
const currentUser = useAuthUser();
const { $toast } = useNuxtApp();
const { impersonate } = useImpersonation();

const { status } = useLazyAsyncData('users', fetchUsers, { server: false });

const search = ref('');
const isSearching = ref(false);
const searchResults = ref<string[] | null>(null);
const sheetOpen = ref(false);
const editingUser = ref<UserData | null>(null);
const sort = ref({ key: 'createdAt', desc: false });
const userQuery = useQueryState('user', '');

const isLoading = computed(() => status.value !== 'success' && !users.value.length);

const columns = [
    { key: 'username', label: 'User', sortable: true, class: 'max-w-0 w-full' },
    { key: 'role', label: 'Role', class: 'hidden md:table-cell' },
    { key: 'files', label: 'Files', sortable: true, class: 'hidden lg:table-cell' },
    { key: 'notes', label: 'Notes', sortable: true, class: 'hidden lg:table-cell' },
    { key: 'storage', label: 'Storage limit', class: 'hidden xl:table-cell' },
    { key: 'createdAt', label: 'Joined', sortable: true, class: 'hidden md:table-cell' },
    { key: 'actions', label: '', class: 'w-12' },
];

const filtered = computed(() =>
    searchResults.value
        ? users.value.filter((u) => searchResults.value!.includes(u.id))
        : users.value,
);

const sorted = computed(() => {
    const values = {
        username: (u: UserData) => u.username.toLowerCase(),
        files: (u: UserData) => u.stats.files,
        notes: (u: UserData) => u.stats.notes,
        createdAt: (u: UserData) => +u.createdAt,
    };

    const getValue = values[sort.value.key as keyof typeof values];

    return [...filtered.value].sort((a, b) => {
        const result = getValue(a) > getValue(b) ? 1 : getValue(a) < getValue(b) ? -1 : 0;
        return sort.value.desc ? -result : result;
    });
});

const getRoles = (user: UserData) =>
    user.superAdmin
        ? ['Super admin']
        : user.permissions.includes(UserPermission.Admin)
          ? ['Admin']
          : user.permissions.map((p) => titleCase(p));

const openSheet = (user: UserData | null) => {
    editingUser.value = user;
    sheetOpen.value = true;
};

const handleImpersonate = async (user: UserData) => {
    try {
        await impersonate(user.username);
    } catch (error: any) {
        $toast.error(error.data.message);
    }
};

const handleDelete = async (user: UserData) => {
    const confirmed = await askConfirmation({
        title: `Delete ${user.username}?`,
        description: 'Their files, folders and notes will be deleted too. This cannot be undone.',
        confirmText: 'Delete',
        destructive: true,
    });

    if (!confirmed) return;

    try {
        const result = await withVerification((verificationData) =>
            $fetch(`/api/users/${user.id}`, { method: 'DELETE', body: { verificationData } }),
        );

        if (result) $toast.success('User deleted');
    } catch (error: any) {
        $toast.error(error.data.message);
    }
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
            searchResults.value = await $fetch<string[]>('/api/users/search', {
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

watch(
    [userQuery, () => users.value.length],
    ([id]) => {
        const user = users.value.find((u) => u.id === id);
        if (!user) return;

        openSheet(user);
        userQuery.value = '';
    },
    { immediate: true },
);

definePageMeta({
    layout: 'dashboard',
    middleware: 'admin-only',
});
</script>
