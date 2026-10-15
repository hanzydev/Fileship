<template>
    <Head>
        <Title>Users</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader
            title="Users"
            :description="isLoading ? 'Loading...' : pluralize(total, 'user')"
        >
            <UiButton @click="openSheet(null)">
                <IUserPlus />
                New user
            </UiButton>
        </PageHeader>

        <div class="flex flex-wrap items-center gap-2">
            <UiInputGroup class="max-w-sm flex-1">
                <UiInputGroupAddon>
                    <UiSpinner v-if="!!search && isFetching" />
                    <ISearch v-else />
                </UiInputGroupAddon>
                <UiInputGroupInput v-model="search" placeholder="Search users..." />
            </UiInputGroup>

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
            title="Couldn't load users"
            description="Something went wrong. Check your connection and try again."
        >
            <UiButton variant="outline" @click="refresh">Try again</UiButton>
        </EmptyState>

        <EmptyState
            v-else-if="!isLoading && !users.length"
            :icon="ISearchX"
            title="No matching users"
            description="Try a different username."
        >
            <UiButton variant="outline" @click="search = ''">Clear search</UiButton>
        </EmptyState>

        <DataTable
            v-else
            v-model:sort="sort"
            :items="users"
            :item-key="(u) => u.id"
            :columns="columns"
            :card-estimate-size="72"
            :loading="isLoading"
            :loading-more="isLoadingMore"
            @reach-end="loadMore"
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
                <span class="block truncate text-muted-foreground">
                    {{
                        item.limits.usableSpace === -1
                            ? 'Unlimited'
                            : `${item.limits.usableSpace} MB`
                    }}
                </span>
            </template>
            <template #cell-createdAt="{ item }">
                <span class="block truncate text-muted-foreground">
                    {{ dayjs(item.createdAt).format('MMM D, YYYY') }}
                </span>
            </template>
            <template #cell-actions="{ item }">
                <div class="flex justify-end">
                    <UserMenu
                        :user="item"
                        @edit="openSheet(item)"
                        @impersonate="handleImpersonate(item)"
                        @delete="handleDelete(item)"
                    />
                </div>
            </template>
            <template #card="{ item }">
                <UiCard size="sm">
                    <UiCardContent class="flex items-center gap-3">
                        <UiAvatar>
                            <UiAvatarImage v-if="item.avatar" :src="item.avatar" />
                            <UiAvatarFallback>
                                {{ item.username.slice(0, 2).toUpperCase() }}
                            </UiAvatarFallback>
                        </UiAvatar>
                        <div class="flex min-w-0 flex-1 flex-col">
                            <TextBlock variant="small" class="truncate">
                                {{ item.username }}
                            </TextBlock>
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
                    </UiCardContent>
                </UiCard>
            </template>
        </DataTable>

        <UserSheet v-model:open="sheetOpen" :user="editingUser" />
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';
import { titleCase } from 'scule';

import { ICircleAlert, ISearchX } from '#components';
import { UserPermission } from '#shared/prisma/enums';

const currentUser = useAuthUser();
const { $toast } = useNuxtApp();
const { impersonate } = useImpersonation();

const search = ref('');
const sheetOpen = ref(false);
const editingUser = ref<UserData | null>(null);
const sort = ref({ key: 'createdAt', desc: true });
const userQuery = useQueryState('user', '');

const {
    items: users,
    total,
    isLoading,
    isError,
    isFetching,
    isLoadingMore,
    loadMore,
    refresh,
} = usePaginatedList<UserData>('users', '/api/users', () => ({
    q: search.value.trim(),
    sort: `${sort.value.key}:${sort.value.desc ? 'desc' : 'asc'}`,
}));

const columns = [
    { key: 'username', label: 'User', sortable: true },
    { key: 'role', label: 'Role', class: 'hidden w-44 lg:table-cell' },
    {
        key: 'files',
        label: 'Files',
        sortable: true,
        descFirst: true,
        class: 'hidden w-24 lg:table-cell',
    },
    {
        key: 'notes',
        label: 'Notes',
        sortable: true,
        descFirst: true,
        class: 'hidden w-24 xl:table-cell',
    },
    { key: 'storage', label: 'Storage limit', class: 'hidden w-36 xl:table-cell' },
    { key: 'createdAt', label: 'Joined', sortable: true, descFirst: true, class: 'w-32' },
    { key: 'actions', label: '', class: 'w-16' },
];

const sortOptions = [
    { label: 'Newest', value: 'createdAt:desc' },
    { label: 'Oldest', value: 'createdAt:asc' },
    { label: 'Name A–Z', value: 'username:asc' },
    { label: 'Name Z–A', value: 'username:desc' },
    { label: 'Most files', value: 'files:desc' },
    { label: 'Most notes', value: 'notes:desc' },
];

const sortValue = computed({
    get: () => `${sort.value.key}:${sort.value.desc ? 'desc' : 'asc'}`,
    set: (value) => {
        const [key, dir] = value.split(':');
        sort.value = { key: key!, desc: dir === 'desc' };
    },
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
        $toast.error(getErrorMessage(error));
    }
};

const handleDelete = async (user: UserData) => {
    await askConfirmation({
        title: `Delete ${user.username}?`,
        description:
            'Their files, folders, notes and short links will be deleted too. This cannot be undone.',
        confirmText: 'Delete',
        destructive: true,
        action: async () => {
            try {
                const result = await withVerification((verificationData) =>
                    $fetch(`/api/users/${user.id}`, {
                        method: 'DELETE',
                        body: { verificationData },
                    }),
                );

                if (result) $toast.success('User deleted');
            } catch (error: any) {
                $toast.error(getErrorMessage(error));
            }
        },
    });
};

watch(
    userQuery,
    async (id) => {
        if (!id) return;

        const user = await $fetch(`/api/users/${id}`).catch(() => null);
        if (user) openSheet(reviveDates(user));

        userQuery.value = '';
    },
    { immediate: true },
);

definePageMeta({
    layout: 'dashboard',
    middleware: 'admin-only',
});
</script>
