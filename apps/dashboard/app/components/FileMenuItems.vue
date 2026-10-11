<template>
    <component :is="Item" @select="emit('view')">
        <IPanelRight />
        View details
    </component>
    <component :is="Item" @select="openFile(file)">
        <IExternalLink />
        Open
    </component>
    <component :is="Item" @select="copyLink(file)">
        <ILink />
        Copy link
    </component>
    <component :is="Item" @select="download([file])">
        <IDownload />
        Download
    </component>
    <component :is="Separator" />
    <component :is="Item" @select="emit('edit')">
        <IPencil />
        Edit
    </component>
    <component :is="Item" @select="emit('move')">
        <IFolderInput />
        {{ file.folderId ? 'Move to another folder' : 'Add to folder' }}
    </component>
    <component :is="Item" v-if="file.folderId" @select="move([file], null)">
        <IFolderMinus />
        Remove from folder
    </component>
    <component :is="Separator" />
    <component :is="Item" variant="destructive" @select="remove([file])">
        <ITrash2 />
        Delete
    </component>
</template>

<script setup lang="ts">
import {
    UiContextMenuItem,
    UiContextMenuSeparator,
    UiDropdownMenuItem,
    UiDropdownMenuSeparator,
} from '#components';

const { file, type } = defineProps<{
    file: FileData;
    type: 'context' | 'dropdown';
}>();

const emit = defineEmits<{
    view: [];
    edit: [];
    move: [];
}>();

const { openFile, copyLink, download, move, remove } = useFileActions();

const Item = type === 'context' ? UiContextMenuItem : UiDropdownMenuItem;
const Separator = type === 'context' ? UiContextMenuSeparator : UiDropdownMenuSeparator;
</script>
