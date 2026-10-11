<template>
    <Head>
        <Title>Notes</Title>
    </Head>

    <div class="flex flex-1 gap-6 p-4 pt-0">
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
                    <ListCardSkeleton v-for="i in 5" :key="i" />
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
                    :estimate-size="66"
                    :gap="8"
                    :loading="isLoadingMore"
                    @reach-end="loadMore"
                >
                    <template #default="{ item }">
                        <UiContextMenu>
                            <UiContextMenuTrigger as-child>
                                <NuxtLink :to="`/dashboard/notes/${item.id}`" class="block">
                                    <UiCard
                                        size="sm"
                                        :class="activeId === item.id && 'ring-2 ring-primary'"
                                    >
                                        <UiCardContent class="flex flex-col gap-0.5">
                                            <div class="flex items-center gap-2">
                                                <TextBlock variant="small" class="flex-1 truncate">
                                                    {{ item.title }}
                                                </TextBlock>
                                                <IGlobe
                                                    v-if="item.public"
                                                    class="size-3.5 text-muted-foreground"
                                                />
                                                <UiDropdownMenu>
                                                    <UiDropdownMenuTrigger as-child>
                                                        <UiButton
                                                            variant="ghost"
                                                            size="icon-sm"
                                                            class="-my-1.5"
                                                            @click.prevent.stop
                                                        >
                                                            <IEllipsis />
                                                        </UiButton>
                                                    </UiDropdownMenuTrigger>
                                                    <UiDropdownMenuContent align="end" class="w-56">
                                                        <NoteMenuItems
                                                            :note="item"
                                                            type="dropdown"
                                                        />
                                                    </UiDropdownMenuContent>
                                                </UiDropdownMenu>
                                            </div>
                                            <TextBlock variant="muted" class="truncate text-xs">
                                                {{ dayjs(item.createdAt).fromNow() }} ·
                                                {{
                                                    item.content
                                                        .replace(/[#*_`>\-\n]+/g, ' ')
                                                        .trim()
                                                }}
                                            </TextBlock>
                                        </UiCardContent>
                                    </UiCard>
                                </NuxtLink>
                            </UiContextMenuTrigger>
                            <UiContextMenuContent class="w-56">
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
