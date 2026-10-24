<template>
    <Head>
        <Title>{{ $t('users.title') }}</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader
            :title="$t('users.title')"
            :description="isLoading ? $t('common.loading') : $t('users.userCount', total)"
        >
            <UiButton @click="openSheet(null)">
                <IUserPlus />
                {{ $t('users.newUser') }}
            </UiButton>
        </PageHeader>

        <div class="flex flex-wrap items-center gap-2">
            <UiInputGroup class="max-w-sm flex-1">
                <UiInputGroupAddon>
                    <UiSpinner v-if="!!search && isFetching" />
                    <ISearch v-else />
                </UiInputGroupAddon>
                <UiInputGroupInput v-model="search" :placeholder="$t('users.searchPlaceholder')" />
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
                    <UiDropdownMenuLabel>{{ $t('users.sortBy') }}</UiDropdownMenuLabel>
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
            :title="$t('users.loadError')"
            :description="$t('users.loadErrorDescription')"
        >
            <UiButton variant="outline" @click="refresh">{{ $t('users.tryAgain') }}</UiButton>
        </EmptyState>

        <EmptyState
            v-else-if="!isLoading && !users.length"
            :icon="ISearchX"
            :title="$t('users.noMatches')"
            :description="$t('users.noMatchesDescription')"
        >
            <UiButton variant="outline" @click="search = ''">
                {{ $t('users.clearSearch') }}
            </UiButton>
        </EmptyState>

        <DataTable
            v-else
            v-model:sort="sort"
            :items="users"
            :item-key="(u) => u.id"
            :columns="columns"
            :card-estimate-size="92"
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
                    <UiBadge v-if="item.id === currentUser!.id" variant="secondary">
                        {{ $t('users.you') }}
                    </UiBadge>
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
                <StorageMeter :used="item.storageUsed" :limit="limitOf(item)" />
            </template>
            <template #cell-createdAt="{ item }">
                <span class="block truncate text-muted-foreground">
                    {{ date(item.createdAt).format('ll') }}
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
                                {{ $t('users.fileCount', item.stats.files) }}
                            </TextBlock>
                            <StorageMeter
                                class="mt-1.5"
                                :used="item.storageUsed"
                                :limit="limitOf(item)"
                            />
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
                            {{
                                editingUser
                                    ? $t('users.editTitle', { username: editingUser.username })
                                    : $t('users.newUser')
                            }}
                        </UiSheetTitle>
                        <UiSheetDescription>
                            {{
                                editingUser
                                    ? $t('users.editDescription')
                                    : $t('users.createDescription')
                            }}
                        </UiSheetDescription>
                    </UiSheetHeader>

                    <div class="flex flex-1 flex-col gap-6 overflow-y-auto px-6 pb-6">
                        <UiFieldGroup>
                            <UiField :data-invalid="!!formErrors?.username">
                                <UiFieldLabel for="user-username">
                                    {{ $t('users.username') }}
                                </UiFieldLabel>
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
                                <UiFieldLabel for="user-password">
                                    {{ $t('users.password') }}
                                </UiFieldLabel>
                                <PasswordInput
                                    id="user-password"
                                    v-model="form.password"
                                    autocomplete="new-password"
                                    :placeholder="
                                        editingUser ? $t('users.passwordKeepPlaceholder') : ''
                                    "
                                />
                                <UiFieldError
                                    v-if="formErrors?.password"
                                    :errors="formErrors.password._errors"
                                />
                            </UiField>
                        </UiFieldGroup>

                        <UiFieldSet v-if="!form.superAdmin">
                            <UiFieldLegend>{{ $t('users.permissions') }}</UiFieldLegend>
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
                                        {{ $t(`users.permission.${permission}`) }}
                                    </UiFieldLabel>
                                </UiField>
                            </UiFieldGroup>
                            <UiFieldError
                                v-if="formErrors?.permissions"
                                :errors="formErrors.permissions._errors"
                            />
                        </UiFieldSet>

                        <UiFieldSet>
                            <UiFieldLegend>{{ $t('users.limits') }}</UiFieldLegend>
                            <UiFieldDescription>
                                {{ $t('users.limitsDescription') }}
                            </UiFieldDescription>
                            <div class="grid grid-cols-2 gap-4">
                                <UiField>
                                    <UiFieldLabel>{{ $t('users.backups') }}</UiFieldLabel>
                                    <UiNumberField v-model="form.limits.backupLimit" :min="-1">
                                        <UiNumberFieldContent>
                                            <UiNumberFieldDecrement />
                                            <UiNumberFieldInput />
                                            <UiNumberFieldIncrement />
                                        </UiNumberFieldContent>
                                    </UiNumberField>
                                </UiField>
                                <UiField>
                                    <UiFieldLabel>{{ $t('users.storageMb') }}</UiFieldLabel>
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
                                <UiFieldLabel for="user-super-admin">
                                    {{ $t('users.superAdmin') }}
                                </UiFieldLabel>
                                <UiFieldDescription>
                                    {{ $t('users.superAdminDescription') }}
                                </UiFieldDescription>
                            </UiFieldContent>
                            <UiSwitch id="user-super-admin" v-model="form.superAdmin" />
                        </UiField>
                    </div>

                    <UiSheetFooter>
                        <UiButton type="submit" :disabled="isSaving">
                            <UiSpinner v-if="isSaving" />
                            {{ editingUser ? $t('users.saveChanges') : $t('users.createUser') }}
                        </UiButton>
                        <UiSheetClose as-child>
                            <UiButton type="button" variant="outline">
                                {{ $t('common.cancel') }}
                            </UiButton>
                        </UiSheetClose>
                    </UiSheetFooter>
                </form>
            </UiSheetContent>
        </UiSheet>
    </div>
</template>

<script setup lang="ts">
import { ICircleAlert, ISearchX } from '#components';
import { UserPermission } from '#shared/prisma/enums';

import type { UserData } from '~/types/users';

const currentUser = useAuthUser();
const { $toast } = useNuxtApp();
const { t } = useI18n();
const date = useDate();
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

const columns = computed(() => [
    { key: 'username', label: t('users.column.user'), sortable: true },
    { key: 'role', label: t('users.column.role'), class: 'hidden w-44 lg:table-cell' },
    {
        key: 'files',
        label: t('users.column.files'),
        sortable: true,
        descFirst: true,
        class: 'hidden w-24 lg:table-cell',
    },
    {
        key: 'notes',
        label: t('users.column.notes'),
        sortable: true,
        descFirst: true,
        class: 'hidden w-24 xl:table-cell',
    },
    { key: 'storage', label: t('users.column.storage'), class: 'hidden w-36 xl:table-cell' },
    {
        key: 'createdAt',
        label: t('users.column.joined'),
        sortable: true,
        descFirst: true,
        class: 'w-32',
    },
    { key: 'actions', label: '', class: 'w-16' },
]);

const sortOptions = computed(() => [
    { label: t('users.sort.newest'), value: 'createdAt:desc' },
    { label: t('users.sort.oldest'), value: 'createdAt:asc' },
    { label: t('users.sort.nameAsc'), value: 'username:asc' },
    { label: t('users.sort.nameDesc'), value: 'username:desc' },
    { label: t('users.sort.mostFiles'), value: 'files:desc' },
    { label: t('users.sort.mostNotes'), value: 'notes:desc' },
]);

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
            $toast.success(user ? t('users.updated') : t('users.created'));
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

const limitOf = (user: UserData) =>
    isAdmin(user) || user.limits.usableSpace < 0 ? null : user.limits.usableSpace * 1024 * 1024;

const getRoles = (user: UserData) =>
    user.superAdmin
        ? [t('users.superAdmin')]
        : user.permissions.includes(UserPermission.Admin)
          ? [t('users.permission.Admin')]
          : user.permissions.map((p) => t(`users.permission.${p}`));

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
        title: t('users.deleteTitle', { username: user.username }),
        description: t('users.deleteDescription'),
        confirmText: t('users.delete'),
        destructive: true,
        action: async () => {
            try {
                const result = await withVerification((verificationData) =>
                    $fetch(`/api/users/${user.id}`, {
                        method: 'DELETE',
                        body: { verificationData },
                    }),
                );

                if (result) $toast.success(t('users.deleted'));
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
