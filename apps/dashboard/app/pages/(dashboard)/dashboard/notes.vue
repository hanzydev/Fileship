<template>
    <Head>
        <Title>Notes</Title>
    </Head>

    <div class="flex flex-1 gap-4 p-4 pt-0">
        <UiCard
            :class="cn('w-full shrink-0 gap-4 md:flex md:w-80', isEditorOpen ? 'hidden' : 'flex')"
        >
            <UiCardHeader>
                <UiCardTitle>Notes</UiCardTitle>
                <UiCardDescription>
                    {{ isLoading ? 'Loading...' : pluralize(total, 'note') }}
                </UiCardDescription>
                <UiCardAction>
                    <UiButton size="sm" as-child>
                        <NuxtLink to="/dashboard/notes/new">
                            <ISquarePen />
                            New
                        </NuxtLink>
                    </UiButton>
                </UiCardAction>
            </UiCardHeader>
            <UiCardContent class="flex flex-1 flex-col gap-3">
                <UiInputGroup>
                    <UiInputGroupAddon>
                        <UiSpinner v-if="!!search && isLoading" />
                        <ISearch v-else />
                    </UiInputGroupAddon>
                    <UiInputGroupInput v-model="search" placeholder="Search notes..." />
                </UiInputGroup>

                <div v-if="isLoading" class="flex flex-col gap-2">
                    <UiSkeleton v-for="i in 5" :key="i" class="h-16 rounded-2xl" />
                </div>

                <EmptyState
                    v-else-if="!notes.length"
                    :icon="search ? ISearchX : IBookmark"
                    :title="search ? 'No matching notes' : 'No notes yet'"
                    :description="search ? 'Try a different search.' : 'Write your first note.'"
                />

                <VirtualList
                    v-else
                    :items="notes"
                    :item-key="(n) => n.id"
                    :estimate-size="64"
                    :gap="4"
                    :loading="isLoadingMore"
                    @reach-end="loadMore"
                >
                    <template #default="{ item }">
                        <UiContextMenu>
                            <UiContextMenuTrigger as-child>
                                <NuxtLink
                                    :to="`/dashboard/notes/${item.id}`"
                                    :class="
                                        cn(
                                            'flex flex-col gap-0.5 rounded-2xl px-3 py-2.5 transition-colors hover:bg-muted',
                                            activeId === item.id && 'bg-muted',
                                        )
                                    "
                                >
                                    <div class="flex items-center gap-2">
                                        <TextBlock variant="small" class="flex-1 truncate">
                                            {{ item.title }}
                                        </TextBlock>
                                        <IGlobe
                                            v-if="item.public"
                                            class="size-3.5 text-muted-foreground"
                                        />
                                    </div>
                                    <TextBlock variant="muted" class="truncate text-xs">
                                        {{ dayjs(item.createdAt).fromNow() }} ·
                                        {{ item.content.replace(/[#*_`>\-\n]+/g, ' ').trim() }}
                                    </TextBlock>
                                </NuxtLink>
                            </UiContextMenuTrigger>
                            <UiContextMenuContent class="w-52">
                                <NoteMenuItems :note="item" type="context" />
                            </UiContextMenuContent>
                        </UiContextMenu>
                    </template>
                </VirtualList>
            </UiCardContent>
        </UiCard>

        <div :class="cn('min-w-0 flex-1 md:flex', isEditorOpen ? 'flex' : 'hidden')">
            <NuxtPage />
        </div>
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

import { IBookmark, ISearchX } from '#components';

const route = useRoute();

const search = ref('');

const {
    items: notes,
    total,
    isLoading,
    isLoadingMore,
    loadMore,
} = usePaginatedList<NoteData>('notes', '/api/notes', () => ({ q: search.value.trim() }));

const activeId = computed(() => (route.params as any).id as string | undefined);
const isEditorOpen = computed(() => !!activeId.value);

definePageMeta({
    layout: 'dashboard',
    middleware: 'note-taker-only',
});
</script>
