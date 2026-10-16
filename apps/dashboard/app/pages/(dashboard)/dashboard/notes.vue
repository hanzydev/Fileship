<template>
    <Head>
        <Title>{{ $t('notes.title') }}</Title>
    </Head>

    <div class="flex flex-1 gap-4 p-4 pt-0">
        <UiCard
            :class="cn('w-full shrink-0 gap-4 md:flex md:w-80', isEditorOpen ? 'hidden' : 'flex')"
        >
            <UiCardHeader>
                <UiCardTitle>{{ $t('notes.title') }}</UiCardTitle>
                <UiCardDescription>
                    {{ isLoading ? $t('common.loading') : $t('notes.count', total) }}
                </UiCardDescription>
                <UiCardAction>
                    <UiButton size="sm" as-child>
                        <NuxtLink to="/dashboard/notes/new">
                            <ISquarePen />
                            {{ $t('notes.new') }}
                        </NuxtLink>
                    </UiButton>
                </UiCardAction>
            </UiCardHeader>
            <UiCardContent class="flex flex-1 flex-col gap-3">
                <UiInputGroup>
                    <UiInputGroupAddon>
                        <UiSpinner v-if="!!search && isFetching" />
                        <ISearch v-else />
                    </UiInputGroupAddon>
                    <UiInputGroupInput
                        v-model="search"
                        :placeholder="$t('notes.searchPlaceholder')"
                    />
                </UiInputGroup>

                <div v-if="isLoading" class="flex flex-col gap-2">
                    <ListCardSkeleton v-for="i in 5" :key="i" />
                </div>

                <EmptyState
                    v-else-if="isError"
                    :icon="ICircleAlert"
                    :title="$t('notes.loadFailed')"
                    :description="$t('notes.loadFailedDescription')"
                >
                    <UiButton variant="outline" @click="refresh">
                        {{ $t('notes.tryAgain') }}
                    </UiButton>
                </EmptyState>

                <EmptyState
                    v-else-if="!notes.length"
                    :icon="search ? ISearchX : IBookmark"
                    :title="search ? $t('notes.noMatches') : $t('notes.empty')"
                    :description="
                        search ? $t('notes.noMatchesDescription') : $t('notes.emptyDescription')
                    "
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
                    <template #skeleton>
                        <ListCardSkeleton />
                    </template>
                    <template #default="{ item }">
                        <UiContextMenu>
                            <UiContextMenuTrigger as-child>
                                <NuxtLink :to="`/dashboard/notes/${item.id}`" class="block">
                                    <UiCard
                                        size="sm"
                                        :class="activeId === item.id && 'ring-2 ring-primary!'"
                                    >
                                        <UiCardContent class="flex items-center gap-2">
                                            <div class="flex min-w-0 flex-1 flex-col gap-0.5">
                                                <TextBlock variant="small" class="truncate">
                                                    {{ item.title }}
                                                </TextBlock>
                                                <TextBlock variant="muted" class="truncate text-xs">
                                                    {{ date(item.createdAt).fromNow() }}
                                                </TextBlock>
                                            </div>
                                            <IGlobe
                                                v-if="item.public"
                                                class="size-3.5 shrink-0 text-muted-foreground"
                                            />
                                            <UiDropdownMenu>
                                                <UiDropdownMenuTrigger as-child>
                                                    <UiButton
                                                        variant="ghost"
                                                        size="icon-sm"
                                                        :aria-label="$t('notes.actions')"
                                                        @click.prevent.stop
                                                    >
                                                        <IEllipsis />
                                                    </UiButton>
                                                </UiDropdownMenuTrigger>
                                                <UiDropdownMenuContent align="end" class="w-56">
                                                    <NoteMenuItems :note="item" type="dropdown" />
                                                </UiDropdownMenuContent>
                                            </UiDropdownMenu>
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
import { IBookmark, ICircleAlert, ISearchX } from '#components';

import type { NoteData } from '~/types/notes';

const route = useRoute();
const date = useDate();

const search = ref('');

const {
    items: notes,
    total,
    isLoading,
    isError,
    isFetching,
    isLoadingMore,
    loadMore,
    refresh,
} = usePaginatedList<NoteData>('notes', '/api/notes', () => ({ q: search.value.trim() }));

const activeId = computed(() => (route.params as any).id as string | undefined);
const isEditorOpen = computed(() => !!activeId.value);

definePageMeta({
    layout: 'dashboard',
    middleware: 'note-taker-only',
});
</script>
