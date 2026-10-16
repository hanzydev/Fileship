<template>
    <component :is="Item" @select="emit('view')">
        <IPanelRight />
        {{ $t('files.viewDetails') }}
    </component>
    <component :is="Item" @select="openFile(file)">
        <IExternalLink />
        {{ $t('files.open') }}
    </component>
    <component :is="Item" @select="copyLink(file)">
        <ILink />
        {{ $t('files.copyLink') }}
    </component>
    <component :is="Item" @select="download([file])">
        <IDownload />
        {{ $t('files.download') }}
    </component>
    <component :is="Separator" />
    <component :is="Item" @select="emit('edit')">
        <IPencil />
        {{ $t('files.edit') }}
    </component>
    <component :is="Item" @select="emit('move')">
        <IFolderInput />
        {{ file.folderId ? $t('files.moveToAnotherFolder') : $t('files.addToFolder') }}
    </component>
    <component :is="Item" v-if="file.folderId" @select="move([file], null)">
        <IFolderMinus />
        {{ $t('files.removeFromFolder') }}
    </component>
    <component
        :is="Item"
        v-if="publicSettings.aiEnabled"
        @select="setSensitive(file, !file.piiDetected)"
    >
        <IShieldCheck v-if="file.piiDetected" />
        <IShieldAlert v-else />
        {{ file.piiDetected ? $t('files.notSensitive') : $t('files.markSensitive') }}
    </component>
    <component :is="Separator" />
    <component :is="Item" variant="destructive" @select="remove([file])">
        <ITrash2 />
        {{ $t('files.delete') }}
    </component>
</template>

<script setup lang="ts">
import {
    UiContextMenuItem,
    UiContextMenuSeparator,
    UiDropdownMenuItem,
    UiDropdownMenuSeparator,
} from '#components';

import type { FileData } from '~/types/files';

const { file, type } = defineProps<{
    file: FileData;
    type: 'context' | 'dropdown';
}>();

const emit = defineEmits<{
    view: [];
    edit: [];
    move: [];
}>();

const publicSettings = usePublicSettings();
const { openFile, copyLink, download, move, remove, setSensitive } = useFileActions();

const Item = type === 'context' ? UiContextMenuItem : UiDropdownMenuItem;
const Separator = type === 'context' ? UiContextMenuSeparator : UiDropdownMenuSeparator;
</script>
