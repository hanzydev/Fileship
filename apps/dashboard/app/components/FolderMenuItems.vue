<template>
    <component :is="Item" v-if="!inFolder" as-child>
        <NuxtLink :to="`/dashboard/folders/${folder.id}`">
            <IFolderOpen />
            {{ $t('folders.menu.open') }}
        </NuxtLink>
    </component>
    <component :is="Item" v-if="folder.public" @select="copyLink(folder)">
        <ILink />
        {{ $t('folders.menu.copyPublicLink') }}
    </component>
    <component :is="Item" v-if="isOwner" @select="togglePublic(folder)">
        <IGlobe v-if="!folder.public" />
        <IGlobeLock v-else />
        {{ folder.public ? $t('folders.menu.makePrivate') : $t('folders.menu.makePublic') }}
    </component>
    <component
        :is="Item"
        :disabled="!folder.fileCount && !folder.folderCount"
        @select="download(folder)"
    >
        <IDownload />
        {{ $t('folders.menu.download') }}
    </component>
    <template v-if="isEditable">
        <component :is="Separator" />
        <component :is="Item" @select="emit('edit')">
            <IPencil />
            {{ $t('folders.menu.edit') }}
        </component>
    </template>
    <template v-if="isOwner">
        <component :is="Item" @select="emit('move')">
            <IFolderInput />
            {{ $t('folders.menu.move') }}
        </component>
        <component :is="Item" @select="emit('share')">
            <IUserPlus />
            {{ $t('folders.menu.share') }}
        </component>
    </template>
    <component :is="Item" v-else @select="emit('share')">
        <IUsers />
        {{ $t('folders.menu.people') }}
    </component>
    <template v-if="leavable">
        <component :is="Separator" />
        <component :is="Item" variant="destructive" @select="leave(folder)">
            <ILogOut />
            {{ $t('folders.menu.leave') }}
        </component>
    </template>
    <template v-else-if="canDelete">
        <component :is="Separator" />
        <component :is="Item" variant="destructive" @select="remove(folder)">
            <ITrash2 />
            {{ $t('folders.menu.delete') }}
        </component>
    </template>
</template>

<script setup lang="ts">
import {
    UiContextMenuItem,
    UiContextMenuSeparator,
    UiDropdownMenuItem,
    UiDropdownMenuSeparator,
} from '#components';

import type { FolderData } from '~/types/folders';

const { folder, type, leavable } = defineProps<{
    folder: FolderData;
    type: 'context' | 'dropdown';
    inFolder?: boolean;
    leavable?: boolean;
}>();

const emit = defineEmits<{
    edit: [];
    move: [];
    share: [];
}>();

const { copyLink, togglePublic, download, leave, remove } = useFolderActions();

const isOwner = computed(() => isOwnedItem(folder));
const isEditable = computed(() => isEditableItem(folder));
const canDelete = computed(
    () =>
        !folder.isInbox && (isOwner.value || (isEditable.value && folder.ancestors?.length !== 0)),
);

const Item = type === 'context' ? UiContextMenuItem : UiDropdownMenuItem;
const Separator = type === 'context' ? UiContextMenuSeparator : UiDropdownMenuSeparator;
</script>
