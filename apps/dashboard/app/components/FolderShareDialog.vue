<template>
    <UiDialog :open="!!folder" @update:open="!$event && (folder = null)">
        <UiDialogContent class="sm:max-w-lg">
            <UiDialogHeader>
                <UiDialogTitle>
                    {{ $t('folders.share.title', { name: shown?.name }) }}
                </UiDialogTitle>
                <UiDialogDescription>{{ $t('folders.share.description') }}</UiDialogDescription>
            </UiDialogHeader>

            <form
                v-if="isOwner"
                class="flex flex-col gap-2 sm:flex-row"
                @submit.prevent="handleAdd"
            >
                <UiField :data-invalid="!!formError" class="flex-1">
                    <UiInput
                        v-model="username"
                        :placeholder="$t('folders.share.usernamePlaceholder')"
                        :aria-label="$t('folders.share.username')"
                        autocomplete="off"
                    />
                    <UiFieldError v-if="formError" :errors="[formError]" />
                </UiField>
                <UiSelect v-model="role">
                    <UiSelectTrigger class="sm:w-32">
                        <UiSelectValue />
                    </UiSelectTrigger>
                    <UiSelectContent>
                        <UiSelectItem
                            v-for="option in roles"
                            :key="option.value"
                            :value="option.value"
                        >
                            {{ option.label }}
                        </UiSelectItem>
                    </UiSelectContent>
                </UiSelect>
                <UiButton type="submit" :disabled="!username.trim() || isAdding">
                    <UiSpinner v-if="isAdding" />
                    <IUserPlus v-else />
                    {{ $t('folders.share.add') }}
                </UiButton>
            </form>

            <div class="flex flex-col gap-2">
                <TextBlock variant="small">{{ $t('folders.share.people') }}</TextBlock>
                <div v-if="isLoading" class="flex flex-col gap-2">
                    <ListCardSkeleton v-for="i in 2" :key="i" media="size-8" />
                </div>
                <TextBlock v-else-if="!members.length" variant="muted">
                    {{ $t('folders.share.empty') }}
                </TextBlock>
                <UiItemGroup v-else class="max-h-80 gap-2 overflow-y-auto">
                    <UiItem
                        v-for="member in members"
                        :key="member.userId"
                        variant="muted"
                        size="sm"
                    >
                        <UiItemMedia>
                            <UiAvatar class="size-8">
                                <UiAvatarImage v-if="member.avatar" :src="member.avatar" />
                                <UiAvatarFallback>
                                    {{ member.username.slice(0, 2).toUpperCase() }}
                                </UiAvatarFallback>
                            </UiAvatar>
                        </UiItemMedia>
                        <UiItemContent class="min-w-0">
                            <UiItemTitle class="truncate">{{ member.username }}</UiItemTitle>
                            <UiItemDescription v-if="!isOwner">
                                {{ roleLabel(member.role) }}
                            </UiItemDescription>
                        </UiItemContent>
                        <UiItemActions v-if="isOwner">
                            <UiSelect
                                :model-value="member.role"
                                :disabled="busyMember === member.userId"
                                @update:model-value="handleRole(member, $event as FolderRole)"
                            >
                                <UiSelectTrigger size="sm" class="w-28">
                                    <UiSelectValue />
                                </UiSelectTrigger>
                                <UiSelectContent>
                                    <UiSelectItem
                                        v-for="option in roles"
                                        :key="option.value"
                                        :value="option.value"
                                    >
                                        {{ option.label }}
                                    </UiSelectItem>
                                </UiSelectContent>
                            </UiSelect>
                            <UiButton
                                variant="ghost"
                                size="icon-sm"
                                :aria-label="$t('folders.share.remove')"
                                :disabled="busyMember === member.userId"
                                @click="handleRemove(member)"
                            >
                                <IX />
                            </UiButton>
                        </UiItemActions>
                    </UiItem>
                </UiItemGroup>
            </div>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
import type { FolderRole } from '#shared/prisma/enums';

import type { FolderData, FolderMemberData } from '~/types/folders';

const folder = defineModel<FolderData | null>({ required: true });

const { $toast } = useNuxtApp();
const { t } = useI18n();

const shown = ref(folder.value);
const members = ref<FolderMemberData[]>([]);
const isLoading = ref(false);
const isAdding = ref(false);
const busyMember = ref<string | null>(null);
const username = ref('');
const role = ref<FolderRole>('Viewer');
const formError = ref('');

const isOwner = computed(() => !!shown.value && isOwnedItem(shown.value));

const roles = computed(() => [
    { label: t('folders.share.viewer'), value: 'Viewer' },
    { label: t('folders.share.editor'), value: 'Editor' },
]);

const roleLabel = (value: FolderRole) =>
    roles.value.find((option) => option.value === value)!.label;

const endpoint = computed(() => `/api/folders/${shown.value!.id}/members`);

const loadMembers = async () => {
    members.value = await $fetch<FolderMemberData[]>(endpoint.value).catch((error) => {
        $toast.error(getErrorMessage(error));
        return [];
    });
};

const saveMember = (name: string, value: FolderRole) =>
    $fetch<FolderMemberData>(endpoint.value, {
        method: 'PUT',
        body: { username: name, role: value },
    });

const handleAdd = async () => {
    isAdding.value = true;
    formError.value = '';

    try {
        const member = await saveMember(username.value.trim(), role.value);

        members.value = [...members.value.filter((m) => m.userId !== member.userId), member];
        username.value = '';
        $toast.success(t('folders.share.added', { username: member.username }));
    } catch (error: any) {
        formError.value = getErrorMessage(error);
    }

    isAdding.value = false;
};

const handleRole = async (member: FolderMemberData, value: FolderRole) => {
    busyMember.value = member.userId;

    try {
        const updated = await saveMember(member.username, value);

        members.value = members.value.map((m) => (m.userId === updated.userId ? updated : m));
        $toast.success(t('folders.share.updated'));
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    busyMember.value = null;
};

const handleRemove = async (member: FolderMemberData) => {
    busyMember.value = member.userId;

    try {
        await $fetch(`${endpoint.value}/${member.userId}`, { method: 'DELETE' });

        members.value = members.value.filter((m) => m.userId !== member.userId);
        $toast.success(t('folders.share.removed'));
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    busyMember.value = null;
};

watch(folder, async (value) => {
    if (!value) return;

    shown.value = value;
    members.value = [];
    username.value = '';
    role.value = 'Viewer';
    formError.value = '';
    isLoading.value = true;
    await loadMembers();
    isLoading.value = false;
});
</script>
