<template>
    <UiContextMenu>
        <UiContextMenuTrigger as-child>
            <UiCard size="sm" class="group" @click="emit('click', $event)">
                <UiCardContent>
                    <div
                        :class="
                            cn(
                                'relative aspect-4/3 overflow-hidden rounded-2xl bg-muted ring-primary transition-shadow',
                                selected && 'ring-2',
                            )
                        "
                    >
                        <img
                            v-if="isImage"
                            :src="file.directUrl"
                            :alt="file.fileName"
                            loading="lazy"
                            :class="
                                cn(
                                    'size-full object-cover transition-[filter,transform] duration-300 group-hover:scale-105',
                                    isSensitive && 'blur-2xl group-hover:blur-none',
                                )
                            "
                        />
                        <img
                            v-else-if="isVideo && file.thumbnailUrl"
                            :src="file.thumbnailUrl"
                            :alt="file.fileName"
                            loading="lazy"
                            class="size-full object-cover transition-transform duration-300 group-hover:scale-105"
                        />
                        <div v-else class="flex size-full items-center justify-center">
                            <component :is="fileIcon" class="size-10 text-muted-foreground" />
                        </div>

                        <div
                            v-if="isVideo"
                            class="absolute inset-0 flex items-center justify-center bg-black/10"
                        >
                            <IPlay class="size-8 fill-white text-white drop-shadow" />
                        </div>

                        <div
                            v-if="isSensitive"
                            class="absolute inset-0 flex flex-col items-center justify-center gap-1 transition-opacity group-hover:opacity-0"
                        >
                            <IShieldAlert class="size-6 text-destructive" />
                            <TextBlock variant="small">Sensitive content</TextBlock>
                        </div>

                        <div
                            :class="
                                cn(
                                    'absolute top-2 left-2 transition-opacity',
                                    !selected &&
                                        !selectionMode &&
                                        'opacity-0 group-hover:opacity-100',
                                )
                            "
                            @click.stop="emit('toggle', $event)"
                        >
                            <UiCheckbox :model-value="selected" />
                        </div>

                        <div
                            class="absolute top-2 right-2 opacity-0 transition-opacity group-hover:opacity-100"
                        >
                            <UiDropdownMenu>
                                <UiDropdownMenuTrigger as-child>
                                    <UiButton variant="secondary" size="icon-sm" @click.stop>
                                        <IEllipsis />
                                    </UiButton>
                                </UiDropdownMenuTrigger>
                                <UiDropdownMenuContent align="end" class="w-56" @click.stop>
                                    <FileMenuItems
                                        :file="file"
                                        type="dropdown"
                                        @view="emit('view')"
                                        @edit="emit('edit')"
                                        @move="emit('move')"
                                    />
                                </UiDropdownMenuContent>
                            </UiDropdownMenu>
                        </div>
                    </div>
                </UiCardContent>
                <UiCardFooter class="flex-col items-start gap-0.5">
                    <div class="flex w-full items-center gap-1.5">
                        <ILock
                            v-if="file.password"
                            class="size-3.5 shrink-0 text-muted-foreground"
                        />
                        <TextBlock variant="small" class="truncate">{{ file.fileName }}</TextBlock>
                    </div>
                    <TextBlock variant="muted" class="truncate text-xs">
                        {{ file.size.formatted }} · {{ dayjs(file.createdAt).fromNow() }}
                    </TextBlock>
                </UiCardFooter>
            </UiCard>
        </UiContextMenuTrigger>
        <UiContextMenuContent>
            <FileMenuItems
                :file="file"
                type="context"
                @view="emit('view')"
                @edit="emit('edit')"
                @move="emit('move')"
            />
        </UiContextMenuContent>
    </UiContextMenu>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

const { file } = defineProps<{
    file: FileData;
    selected?: boolean;
    selectionMode?: boolean;
}>();

const emit = defineEmits<{
    click: [event: MouseEvent];
    toggle: [event: MouseEvent];
    view: [];
    edit: [];
    move: [];
}>();

const currentUser = useAuthUser();
const runtimeConfig = useRuntimeConfig();

const isImage = computed(() => file.mimeType.startsWith('image/'));
const isVideo = computed(() => file.mimeType.startsWith('video/'));

const isSensitive = computed(
    () =>
        isImage.value &&
        file.piiDetected &&
        currentUser.value!.aiSettings.suppressPii &&
        runtimeConfig.public.aiEnabled,
);

const fileIcon = computed(() => getFileIcon(file));
</script>
