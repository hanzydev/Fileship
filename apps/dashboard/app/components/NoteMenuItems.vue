<template>
    <component :is="Item" v-if="!inNote" as-child>
        <NuxtLink :to="`/dashboard/notes/${note.id}`">
            <IBookOpen />
            Open
        </NuxtLink>
    </component>
    <component :is="Item" v-if="note.public && note.publicUrl" @select="copyLink(note)">
        <ILink />
        Copy public link
    </component>
    <component :is="Item" @select="copyContent(note)">
        <ICopy />
        Copy content
    </component>
    <component :is="Item" @select="togglePublic(note)">
        <IGlobe v-if="!note.public" />
        <IGlobeLock v-else />
        {{ note.public ? 'Make private' : 'Make public' }}
    </component>
    <component :is="Separator" />
    <component :is="Item" variant="destructive" @select="remove(note)">
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
