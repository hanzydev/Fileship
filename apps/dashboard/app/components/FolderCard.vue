<template>
    <UiContextMenu>
        <UiContextMenuTrigger as-child>
            <NuxtLink :to="`/dashboard/folders/${folder.id}`">
                <UiCard size="sm" class="group">
                    <UiCardContent>
                        <div
                            class="relative grid aspect-video grid-cols-2 grid-rows-2 gap-1 overflow-hidden rounded-2xl bg-muted"
                        >
                            <img
                                v-for="(file, index) in folder.covers"
                                :key="file.id"
                                :src="file.url"
                                :alt="file.fileName"
                                loading="lazy"
                                :class="
                                    cn(
                                        'size-full object-cover transition-transform duration-300',
                                        folder.public
                                            ? 'group-hover:scale-105'
                                            : 'scale-125 blur-2xl',
                                        folder.covers?.length === 1 && 'col-span-2 row-span-2',
                                        folder.covers?.length === 2 && 'row-span-2',
                                        folder.covers?.length === 3 && index === 0 && 'row-span-2',
                                    )
                                "
                            />
                            <div
                                v-if="folder.covers?.length && !folder.public"
                                class="absolute inset-0 flex items-center justify-center"
                            >
                                <div
                                    class="flex size-10 items-center justify-center rounded-full bg-background/60 backdrop-blur-md"
                                >
                                    <ILock class="size-4" />
                                </div>
                            </div>
                            <div
                                v-if="!folder.covers?.length"
                                class="col-span-2 row-span-2 flex items-center justify-center"
                            >
                                <IInbox
                                    v-if="folder.isInbox"
                                    class="size-10 text-muted-foreground"
                                />
                                <IFolder v-else class="size-10 text-muted-foreground" />
                            </div>
                        </div>
                    </UiCardContent>
                    <UiCardFooter class="gap-2">
                        <div class="flex min-w-0 flex-1 flex-col gap-0.5">
                            <TextBlock variant="small" class="truncate">
                                {{ folder.name }}
                            </TextBlock>
                            <TextBlock variant="muted" class="truncate text-xs">
                                {{ details }}
                            </TextBlock>
                        </div>
                        <UiBadge v-if="folder.isInbox" variant="secondary">
                            {{ $t('folders.inbox') }}
                        </UiBadge>
                        <UiBadge v-if="folder.public" variant="outline">
                            <IGlobe />
                            {{ $t('folders.public') }}
                        </UiBadge>
                        <UiBadge v-if="folder.memberCount && isOwnedItem(folder)" variant="outline">
                            <IUsers />
                            {{ $t('folders.shared') }}
                        </UiBadge>
                        <UiDropdownMenu>
                            <UiDropdownMenuTrigger as-child>
                                <UiButton variant="ghost" size="icon-sm" @click.prevent.stop>
                                    <IEllipsis />
                                </UiButton>
                            </UiDropdownMenuTrigger>
                            <UiDropdownMenuContent align="end" class="w-56">
                                <FolderMenuItems
                                    :folder="folder"
                                    type="dropdown"
                                    :leavable="leavable"
                                    @edit="emit('edit')"
                                    @move="emit('move')"
                                    @share="emit('share')"
                                />
                            </UiDropdownMenuContent>
                        </UiDropdownMenu>
                    </UiCardFooter>
                </UiCard>
            </NuxtLink>
        </UiContextMenuTrigger>
        <UiContextMenuContent class="w-56">
            <FolderMenuItems
                :folder="folder"
                type="context"
                :leavable="leavable"
                @edit="emit('edit')"
                @move="emit('move')"
                @share="emit('share')"
            />
        </UiContextMenuContent>
    </UiContextMenu>
</template>

<script setup lang="ts">
import type { FolderData } from '~/types/folders';

const { folder, showParent, leavable } = defineProps<{
    folder: FolderData;
    showParent?: boolean;
    leavable?: boolean;
}>();

const emit = defineEmits<{
    edit: [];
    move: [];
    share: [];
}>();

const { t } = useI18n();

const details = computed(() => {
    const files = t('folders.fileCount', folder.fileCount);
    const contents = folder.folderCount
        ? t('folders.contents', { files, folders: t('folders.count', folder.folderCount) })
        : files;

    return showParent && folder.parent
        ? t('folders.inParent', { contents, parent: folder.parent.name })
        : contents;
});
</script>
