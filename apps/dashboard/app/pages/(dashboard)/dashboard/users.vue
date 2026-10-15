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

        <UiSheet v-model:open="sheetOpen">
            <UiSheetContent class="data-[side=right]:w-full data-[side=right]:sm:max-w-md">
                <form class="flex flex-1 flex-col overflow-hidden" @submit.prevent="handleSubmit">
                    <UiSheetHeader>
                        <UiSheetTitle>
                            {{ editingUser ? `Edit ${editingUser.username}` : 'New user' }}
                        </UiSheetTitle>
                        <UiSheetDescription>
                            {{
                                editingUser
                                    ? 'Update account details, permissions and limits.'
                                    : 'Create an account for someone else.'
                            }}
                        </UiSheetDescription>
                    </UiSheetHeader>

                    <div class="flex flex-1 flex-col gap-6 overflow-y-auto px-6 pb-6">
                        <UiFieldGroup>
                            <UiField :data-invalid="!!formErrors?.username">
                                <UiFieldLabel for="user-username">Username</UiFieldLabel>
                                <UiInput
                                    id="user-username"
                                    v-model="form.username"
                                    autocomplete="off"
                                />
                                <UiFieldError
                                    v-if="formErrors?.username"
                                    :errors="formErrors.username._errors"
                                />
                            </UiField>
                            <UiField :data-invalid="!!formErrors?.password">
                                <UiFieldLabel for="user-password">Password</UiFieldLabel>
                                <PasswordInput
                                    id="user-password"
                                    v-model="form.password"
                                    autocomplete="new-password"
                                    :placeholder="
                                        editingUser ? 'Leave empty to keep the current one' : ''
                                    "
                                />
                                <UiFieldError
                                    v-if="formErrors?.password"
                                    :errors="formErrors.password._errors"
                                />
                            </UiField>
                        </UiFieldGroup>

                        <UiFieldSet v-if="!form.superAdmin">
                            <UiFieldLegend>Permissions</UiFieldLegend>
                            <UiFieldGroup class="gap-3">
                                <UiField
                                    v-for="permission in Object.values(UserPermission)"
                                    :key="permission"
                                    orientation="horizontal"
                                >
                                    <UiCheckbox
                                        :id="`permission-${permission}`"
                                        :model-value="form.permissions.includes(permission)"
                                        :disabled="
                                            permission !== UserPermission.Admin &&
                                            form.permissions.includes(UserPermission.Admin)
                                        "
                                        @update:model-value="togglePermission(permission)"
                                    />
                                    <UiFieldLabel :for="`permission-${permission}`">
                                        {{ titleCase(permission) }}
                                    </UiFieldLabel>
                                </UiField>
                            </UiFieldGroup>
                            <UiFieldError
                                v-if="formErrors?.permissions"
                                :errors="formErrors.permissions._errors"
                            />
                        </UiFieldSet>

                        <UiFieldSet>
                            <UiFieldLegend>Limits</UiFieldLegend>
                            <UiFieldDescription>Use -1 for unlimited.</UiFieldDescription>
                            <div class="grid grid-cols-2 gap-4">
                                <UiField>
                                    <UiFieldLabel>Backups</UiFieldLabel>
                                    <UiNumberField v-model="form.limits.backupLimit" :min="-1">
                                        <UiNumberFieldContent>
                                            <UiNumberFieldDecrement />
                                            <UiNumberFieldInput />
                                            <UiNumberFieldIncrement />
                                        </UiNumberFieldContent>
                                    </UiNumberField>
                                </UiField>
                                <UiField>
                                    <UiFieldLabel>Storage (MB)</UiFieldLabel>
                                    <UiNumberField v-model="form.limits.usableSpace" :min="-1">
                                        <UiNumberFieldContent>
                                            <UiNumberFieldDecrement />
                                            <UiNumberFieldInput />
                                            <UiNumberFieldIncrement />
                                        </UiNumberFieldContent>
                                    </UiNumberField>
                                </UiField>
                            </div>
                        </UiFieldSet>

                        <UiField v-if="currentUser!.superAdmin" orientation="horizontal">
                            <UiFieldContent>
                                <UiFieldLabel for="user-super-admin">Super admin</UiFieldLabel>
                                <UiFieldDescription>
                                    Full access, including managing other admins.
                                </UiFieldDescription>
                            </UiFieldContent>
                            <UiSwitch id="user-super-admin" v-model="form.superAdmin" />
                        </UiField>
                    </div>

                    <UiSheetFooter>
                        <UiButton type="submit" :disabled="isSaving">
                            <UiSpinner v-if="isSaving" />
                            {{ editingUser ? 'Save changes' : 'Create user' }}
                        </UiButton>
                        <UiSheetClose as-child>
                            <UiButton type="button" variant="outline">Cancel</UiButton>
                        </UiSheetClose>
                    </UiSheetFooter>
                </form>
            </UiSheetContent>
        </UiSheet>
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';
import { titleCase } from 'scule';

import { ICircleAlert, ISearchX } from '#components';
import { UserPermission } from '#shared/prisma/enums';

import type { UserData } from '~/types/users';

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

const isSaving = ref(false);
const formErrors = ref();

const form = reactive({
    username: '',
    password: '',
    permissions: [] as UserPermission[],
    superAdmin: false,
    limits: { backupLimit: -1, usableSpace: -1 },
});

const togglePermission = (permission: UserPermission) => {
    if (form.permissions.includes(permission)) {
        form.permissions = form.permissions.filter((p) => p !== permission);
    } else {
        form.permissions =
            permission === UserPermission.Admin ? [permission] : [...form.permissions, permission];
    }
};

const handleSubmit = async () => {
    const user = editingUser.value;

    isSaving.value = true;
    formErrors.value = null;

    const body = {
        ...form,
        password: user && !form.password ? undefined : form.password,
        permissions: form.superAdmin ? [] : form.permissions,
    };

    try {
        const result = await withVerification<unknown>((verificationData) =>
            user
                ? $fetch(`/api/users/${user.id}`, {
                      method: 'PATCH',
                      body: { ...body, verificationData },
                  })
                : $fetch('/api/users', { method: 'POST', body: { ...body, verificationData } }),
        );

        if (result) {
            sheetOpen.value = false;
            $toast.success(user ? 'User updated' : 'User created');
        }
    } catch (error: any) {
        formErrors.value = error.data?.data?.formErrors;
        if (!formErrors.value) $toast.error(getErrorMessage(error));
    }

    isSaving.value = false;
};

watch(sheetOpen, (value) => {
    if (!value) return;

    formErrors.value = null;
    Object.assign(form, {
        username: editingUser.value?.username || '',
        password: '',
        permissions: [...(editingUser.value?.permissions || [])],
        superAdmin: editingUser.value?.superAdmin || false,
        limits: { ...defaultUserLimits, ...(editingUser.value?.limits as any) },
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
