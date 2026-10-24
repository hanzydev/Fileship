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
    <component :is="Item" @select="togglePublic(folder)">
        <IGlobe v-if="!folder.public" />
        <IGlobeLock v-else />
        {{ folder.public ? $t('folders.menu.makePrivate') : $t('folders.menu.makePublic') }}
    </component>
    <component :is="Item" :disabled="!folder.fileCount" @select="download(folder)">
        <IDownload />
        {{ $t('folders.menu.download') }}
    </component>
    <component :is="Separator" />
    <component :is="Item" @select="emit('edit')">
        <IPencil />
        {{ $t('folders.menu.edit') }}
    </component>
    <component :is="Item" @select="emit('move')">
        <IFolderInput />
        {{ $t('folders.menu.move') }}
    </component>
    <template v-if="!folder.isInbox">
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

const { folder, type } = defineProps<{
    folder: FolderData;
    type: 'context' | 'dropdown';
    inFolder?: boolean;
}>();

const emit = defineEmits<{
    edit: [];
    move: [];
}>();

const { copyLink, togglePublic, download, remove } = useFolderActions();

const Item = type === 'context' ? UiContextMenuItem : UiDropdownMenuItem;
const Separator = type === 'context' ? UiContextMenuSeparator : UiDropdownMenuSeparator;
</script>
