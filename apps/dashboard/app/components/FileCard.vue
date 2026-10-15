<template>
    <UiContextMenu>
        <UiContextMenuTrigger as-child :disabled="pickable">
            <UiCard
                v-bind="$attrs"
                size="sm"
                class="group"
                tabindex="0"
                @click="emit('click', $event)"
                @keydown.enter.self="emit('click', $event)"
            >
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
                            ref="media"
                            :src="file.thumbnailUrl || file.directUrl"
                            :alt="file.name"
                            loading="lazy"
                            decoding="async"
                            :class="
                                cn(
                                    'size-full object-cover transition-[filter,scale] duration-300 group-hover:scale-105',
                                    isSensitive && 'blur-2xl group-hover:blur-none',
                                )
                            "
                            @load="isLoaded = true"
                            @error="isLoaded = true"
                        />
                        <img
                            v-else-if="isVideo && file.thumbnailUrl"
                            ref="media"
                            :src="file.thumbnailUrl"
                            :alt="file.name"
                            loading="lazy"
                            class="size-full object-cover transition-transform duration-300 group-hover:scale-105"
                            @load="isLoaded = true"
                            @error="isLoaded = true"
                        />
                        <div v-else class="flex size-full items-center justify-center">
                            <component :is="fileIcon" class="size-10 text-muted-foreground" />
                        </div>
                        <UiSkeleton v-if="hasMedia && !isLoaded" class="absolute inset-0" />

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
                                    chipClass,
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
                            v-if="!pickable"
                            :class="
                                cn(
                                    'absolute top-2 right-2 opacity-0 transition-opacity group-hover:opacity-100',
                                    chipClass,
                                )
                            "
                        >
                            <UiDropdownMenu>
                                <UiDropdownMenuTrigger as-child>
                                    <UiButton
                                        variant="ghost"
                                        size="icon-sm"
                                        aria-label="More"
                                        @click.stop
                                    >
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
                <UiCardFooter class="gap-2">
                    <div class="flex min-w-0 flex-1 flex-col gap-0.5">
                        <div class="flex w-full items-center gap-1.5">
                            <ILock
                                v-if="file.password"
                                class="size-3.5 shrink-0 text-muted-foreground"
                            />
                            <TextBlock variant="small" class="truncate">{{ file.name }}</TextBlock>
                        </div>
                        <TextBlock variant="muted" class="truncate text-xs">
                            {{ file.size.formatted }} · {{ dayjs(file.createdAt).fromNow() }}
                        </TextBlock>
                    </div>
                    <UiTooltip v-if="file.piiDetected">
                        <UiTooltipTrigger as-child>
                            <UiBadge variant="destructive" aria-label="Sensitive">
                                <IShieldAlert />
                            </UiBadge>
                        </UiTooltipTrigger>
                        <UiTooltipContent>Sensitive · hidden from visitors</UiTooltipContent>
                    </UiTooltip>
                </UiCardFooter>
            </UiCard>
        </UiContextMenuTrigger>
        <UiContextMenuContent v-if="!pickable" class="w-56">
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

import type { FileData } from '~/types/files';

defineOptions({ inheritAttrs: false });

const { file } = defineProps<{
    file: FileData;
    selected?: boolean;
    selectionMode?: boolean;
    pickable?: boolean;
}>();

const chipClass =
    'flex size-8 items-center justify-center rounded-full bg-[color-mix(in_oklch,var(--secondary),var(--foreground)_20%)]/90 backdrop-blur-md';

const emit = defineEmits<{
    click: [event: MouseEvent | KeyboardEvent];
    toggle: [event: MouseEvent];
    view: [];
    edit: [];
    move: [];
}>();

const currentUser = useAuthUser();
const publicSettings = usePublicSettings();

const isImage = computed(() => file.mimeType.startsWith('image/'));
const isVideo = computed(() => file.mimeType.startsWith('video/'));
const hasMedia = computed(() => isImage.value || (isVideo.value && !!file.thumbnailUrl));

const media = useTemplateRef<HTMLImageElement>('media');
const isLoaded = ref(false);

const isSensitive = computed(
    () =>
        isImage.value &&
        file.piiDetected &&
        currentUser.value!.aiSettings.suppressPii &&
        publicSettings.value.aiEnabled,
);

const fileIcon = computed(() => getFileIcon(file));

onMounted(() => {
    if (media.value?.complete) isLoaded.value = true;
});
</script>
