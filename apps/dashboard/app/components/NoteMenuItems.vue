<template>
    <component :is="Item" v-if="!inNote" as-child>
        <NuxtLink :to="`/dashboard/notes/${note.id}`">
            <IBookOpen />
            {{ $t('notes.menu.open') }}
        </NuxtLink>
    </component>
    <component :is="Item" v-if="note.public && note.publicUrl" @select="copyLink(note)">
        <ILink />
        {{ $t('notes.menu.copyLink') }}
    </component>
    <component :is="Item" @select="copyContent(note)">
        <ICopy />
        {{ $t('notes.menu.copyContent') }}
    </component>
    <component :is="Item" @select="togglePublic(note)">
        <IGlobe v-if="!note.public" />
        <IGlobeLock v-else />
        {{ note.public ? $t('notes.menu.makePrivate') : $t('notes.menu.makePublic') }}
    </component>
    <component :is="Separator" />
    <component :is="Item" variant="destructive" @select="remove(note)">
        <ITrash2 />
        {{ $t('notes.menu.delete') }}
    </component>
</template>

<script setup lang="ts">
import {
    UiContextMenuItem,
    UiContextMenuSeparator,
    UiDropdownMenuItem,
    UiDropdownMenuSeparator,
} from '#components';

import type { NoteData } from '~/types/notes';

const { note, type } = defineProps<{
    note: NoteData;
    type: 'context' | 'dropdown';
    inNote?: boolean;
}>();

const { copyLink, copyContent, togglePublic, remove } = useNoteActions();

const Item = type === 'context' ? UiContextMenuItem : UiDropdownMenuItem;
const Separator = type === 'context' ? UiContextMenuSeparator : UiDropdownMenuSeparator;
</script>
