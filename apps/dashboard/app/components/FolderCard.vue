<template>
    <UiContextMenu>
        <UiContextMenuTrigger as-child>
            <NuxtLink :to="`/dashboard/folders/${folder.id}`">
                <UiCard size="sm" class="group">
                    <UiCardContent>
                        <div
                            class="grid aspect-video grid-cols-2 grid-rows-2 gap-1 overflow-hidden rounded-2xl bg-muted"
                        >
                            <img
                                v-for="(file, index) in covers"
                                :key="file.id"
                                :src="file.thumbnailUrl || file.directUrl"
                                :alt="file.fileName"
                                loading="lazy"
                                :class="
                                    cn(
                                        'size-full object-cover',
                                        covers.length === 1 && 'col-span-2 row-span-2',
                                        covers.length === 2 && 'row-span-2',
                                        covers.length === 3 && index === 0 && 'row-span-2',
                                    )
                                "
                            />
                            <div
                                v-if="!covers.length"
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
                            <TextBlock variant="muted" class="text-xs">
                                {{ pluralize(folder.files.length, 'file') }}
                            </TextBlock>
                        </div>
                        <UiBadge v-if="folder.isInbox" variant="secondary">Inbox</UiBadge>
                        <UiBadge v-if="folder.public" variant="outline">
                            <IGlobe />
                            Public
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
                                    @edit="emit('edit')"
                                />
                            </UiDropdownMenuContent>
                        </UiDropdownMenu>
                    </UiCardFooter>
                </UiCard>
            </NuxtLink>
        </UiContextMenuTrigger>
        <UiContextMenuContent>
            <FolderMenuItems :folder="folder" type="context" @edit="emit('edit')" />
        </UiContextMenuContent>
    </UiContextMenu>
</template>

<script setup lang="ts">
const { folder } = defineProps<{
    folder: FolderData;
}>();

const emit = defineEmits<{
    edit: [];
}>();

const files = useFiles();

const covers = computed(() =>
    files.value
        .filter(
            (f) =>
                f.folderId === folder.id &&
                (f.mimeType.startsWith('image/') ||
                    (f.mimeType.startsWith('video/') && f.thumbnailUrl)) &&
                !f.piiDetected,
        )
        .slice(0, 4),
);
</script>
