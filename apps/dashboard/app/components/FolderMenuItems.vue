<template>
    <component :is="Item" v-if="!inFolder" as-child>
        <NuxtLink :to="`/dashboard/folders/${folder.id}`">
            <IFolderOpen />
            Open
        </NuxtLink>
    </component>
    <component :is="Item" v-if="folder.public" @select="copyLink(folder)">
        <ILink />
        Copy public link
    </component>
    <component :is="Item" @select="togglePublic(folder)">
        <IGlobe v-if="!folder.public" />
        <IGlobeLock v-else />
        {{ folder.public ? 'Make private' : 'Make public' }}
    </component>
    <component :is="Item" :disabled="!folder.fileCount" @select="download(folder)">
        <IDownload />
        Download as archive
    </component>
    <component :is="Separator" />
    <component :is="Item" @select="emit('edit')">
        <IPencil />
        Edit
    </component>
    <template v-if="!folder.isInbox">
        <component :is="Separator" />
        <component :is="Item" variant="destructive" @select="remove(folder)">
            <ITrash2 />
            Delete
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

const { folder, type } = defineProps<{
    folder: FolderData;
    type: 'context' | 'dropdown';
    inFolder?: boolean;
}>();

const emit = defineEmits<{
    edit: [];
}>();

const { copyLink, togglePublic, download, remove } = useFolderActions();

const Item = type === 'context' ? UiContextMenuItem : UiDropdownMenuItem;
const Separator = type === 'context' ? UiContextMenuSeparator : UiDropdownMenuSeparator;
</script>
