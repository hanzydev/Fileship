<template>
    <template v-if="item.url">
        <component :is="Item" @select="emit('select', 'history:copy')">
            <ICopy />
            {{ $t('home.copyLink') }}
        </component>
        <component :is="Item" @select="emit('select', 'history:open')">
            <IExternalLink />
            {{ $t('home.openLink') }}
        </component>
    </template>
    <component :is="Item" v-if="item.fileId" @select="emit('view')">
        <IPanelRight />
        {{ $t('home.viewInFileship') }}
    </component>
    <component :is="Item" v-if="item.localPath" @select="emit('select', 'history:showFile')">
        <IFolderOpen />
        {{ $t('home.showInFolder') }}
    </component>
    <component
        :is="Item"
        v-if="isHarborReady && item.fileId"
        @select="emit('select', 'history:showInHarbor')"
    >
        <IAnchor />
        {{ $t('home.showInHarbor') }}
    </component>
    <component :is="Separator" v-if="item.url || item.fileId || item.localPath" />
    <component :is="Item" variant="destructive" @select="emit('select', 'history:remove')">
        <ITrash2 />
        {{ $t('home.removeFromHistory') }}
    </component>
</template>

<script setup lang="ts">
import {
    UiContextMenuItem,
    UiContextMenuSeparator,
    UiDropdownMenuItem,
    UiDropdownMenuSeparator,
} from '#components';

import type { HistoryItem } from '../../shared/ipc';

export type HistoryChannel =
    | 'history:copy'
    | 'history:open'
    | 'history:showFile'
    | 'history:showInHarbor'
    | 'history:remove';

const { item, type } = defineProps<{
    item: HistoryItem;
    type: 'context' | 'dropdown';
    isHarborReady: boolean;
}>();

const emit = defineEmits<{
    select: [channel: HistoryChannel];
    view: [];
}>();

const Item = type === 'context' ? UiContextMenuItem : UiDropdownMenuItem;
const Separator = type === 'context' ? UiContextMenuSeparator : UiDropdownMenuSeparator;
</script>
