<template>
    <div>
        <ModalsEditFile
            v-if="currentUser?.id === data.authorId && !partial"
            v-model="editModalOpen"
            :data
        />

        <UiModal
            v-if="isImage || isVideo || isAudio"
            v-model="isOpen"
            wscreen="!"
            ring-0="!"
            bg-transparent="!"
            flex="~ col justify-between"
            max-hscreen
            max-wfull
            min-hscreen
            overflow-hidden
            rounded-none
            p8
            @click.self="isOpen = false"
        >
            <div
                flex="~ gap-4 md:(justify-between items-center) <md:col"
                z20
                wfull
                motion-safe="transition-opacity"
                :class="zoom.zoomedIn && 'opacity-0'"
            >
                <div space-y-2>
                    <h3 line-clamp-2 break-all>{{ data.fileName }}</h3>
                    <div flex="~ gap2 wrap">
                        <div v-if="data.views" flex="~ items-center gap2">
                            <Icon name="heroicons-solid:eye" size="24" />
                            <span font-medium="!">{{ data.views.today }} today</span>
                        </div>
                        <div flex="~ items-center gap2">
                            <Icon name="mdi:sd-storage" size="24" />
                            <span font-medium="!">{{ data.size!.formatted }}</span>
                        </div>
                        <div flex="~ items-center gap2">
                            <Icon name="heroicons-solid:calendar" size="24" />
                            <span font-medium="!">
                                {{ dayjs(data.createdAt).fromNow() }}
                            </span>
                        </div>
                    </div>
                </div>
                <div flex="~ gap2.5" hfit>
                    <div
                        flex="~ items-center justify-center gap-1"
                        ring="1 fs-overlay-4"
                        rounded-lg
                        bg-fs-overlay-2
                        px1
                    >
                        <UiButton
                            alignment="center"
                            variant="onOverlay"
                            class="size-9 shrink-0 text-fs-muted-2 !rounded !p0 hover:text-white"
                            icon="heroicons-solid:download"
                            icon-size="24"
                            :href="`${data.directUrl}?download`"
                            target="_blank"
                        />

                        <UiButton
                            alignment="center"
                            variant="onOverlay"
                            class="size-9 shrink-0 text-fs-muted-2 !rounded !p0 hover:text-white"
                            :icon="
                                copied
                                    ? 'heroicons-solid:clipboard-check'
                                    : 'heroicons-solid:clipboard-copy'
                            "
                            :icon-class="copied && 'text-green500'"
                            icon-size="20"
                            @click="handleCopy"
                        />

                        <template v-if="currentUser?.id === data.authorId && !partial">
                            <UiButton
                                v-if="data.folderId"
                                alignment="center"
                                variant="onOverlay"
                                class="size-9 shrink-0 text-fs-muted-2 !rounded !p0 hover:text-white"
                                icon="heroicons-solid:folder-remove"
                                icon-size="24"
                                :disabled="takingOut"
                                :loading="takingOut"
                                @click="handleTakeOut"
                            />
                            <UiButton
                                alignment="center"
                                variant="onOverlay"
                                class="size-9 shrink-0 text-fs-muted-2 !rounded !p0 hover:text-white"
                                icon="heroicons:pencil-16-solid"
                                icon-size="24"
                                @click="editModalOpen = true"
                            />
                            <UiButton
                                alignment="center"
                                class="size-9 shrink-0 text-fs-muted-2 !rounded !p0 !hover:(bg-red-600 text-white)"
                                variant="onOverlay"
                                icon="heroicons-solid:trash"
                                icon-size="20"
                                :disabled="deleting"
                                :loading="deleting"
                                @click="handleDelete"
                            />
                        </template>
                    </div>

                    <UiButton
                        variant="accent"
                        alignment="center"
                        class="size-11 shrink-0 text-fs-muted-2 !p0 hover:text-white"
                        icon="heroicons-solid:x"
                        icon-size="24"
                        @click="isOpen = false"
                    />
                </div>
            </div>
            <div
                flex="~ justify-between items-center"
                relative
                :class="mediaFiles.length <= 1 && 'mya'"
                @click.self="isOpen = false"
            >
                <UiButton
                    v-if="files.length > 1"
                    variant="secondary"
                    alignment="center"
                    icon="heroicons-solid:arrow-narrow-left"
                    icon-size="24"
                    aria-label="Previous"
                    data-ignore-modal-outer-click
                    :class="[
                        'z20 size-11 shrink-0 text-fs-muted-2 !p0 hover:text-white motion-safe:transition-opacity',
                        zoom.zoomedIn && 'opacity-0',
                    ]"
                    :disabled="isFirst || !prev"
                    @click="handlePrev"
                />

                <div :id="modalId" flex="~ items-center justify-center" relative>
                    <template v-if="isImage">
                        <img
                            v-if="reducedMotion === 'no-preference'"
                            flex="~ items-center justify-center'"
                            absolute
                            z10
                            select-none
                            blur-3xl
                            motion-safe="transition-transform duration-300"
                            lt-lg="max-w-[calc((100%-44px*2)-16px)]"
                            md="max-h-75vh"
                            :class="zoom.zoomedIn ? 'cursor-zoom-out scale-250' : 'cursor-zoom-in'"
                            :style="{
                                transformOrigin: `${zoom.originX} ${zoom.originY}`,
                            }"
                            :src="data.directUrl"
                        />

                        <img
                            z11
                            select-none
                            motion-safe="transition-transform duration-300"
                            lt-lg="max-w-[calc((100%-44px*2)-16px)]"
                            md="max-h-75vh"
                            :class="zoom.zoomedIn ? 'cursor-zoom-out scale-250' : 'cursor-zoom-in'"
                            :style="{
                                transformOrigin: `${zoom.originX} ${zoom.originY}`,
                            }"
                            :src="data.directUrl"
                            :alt="data.fileName"
                            @click="handleZoom"
                        />
                    </template>
                    <template v-else-if="isVideo">
                        <canvas
                            ref="canvasRef"
                            absolute
                            z10
                            max-w="[calc((100%-44px*2)-16px)]"
                            max-h="[calc(75vh-16px)]"
                            blur-3xl
                        />
                        <video
                            ref="videoRef"
                            :src="data.directUrl"
                            :poster="data.thumbnailUrl !== null ? data.thumbnailUrl : undefined"
                            controls
                            relative
                            z11
                            wauto
                            max-w="[calc((100%-44px*2)-16px)]"
                            max-h="[calc(75vh-16px)]"
                            rounded-lg
                        />
                    </template>
                    <audio
                        v-else
                        :src="data.directUrl"
                        controls
                        w="[calc((100%-44px*2)-16px)]"
                        lg="w-200"
                    />
                </div>

                <UiButton
                    v-if="files.length > 1"
                    variant="secondary"
                    alignment="center"
                    icon="heroicons-solid:arrow-narrow-right"
                    icon-size="24"
                    aria-label="Next"
                    data-ignore-modal-outer-click
                    :class="[
                        'z20 size-11 shrink-0 text-fs-muted-2 !p0 hover:text-white motion-safe:transition-opacity',
                        zoom.zoomedIn && 'opacity-0',
                    ]"
                    :disabled="isLast || !next"
                    @click="handleNext"
                />
            </div>

            <div
                v-if="mediaFiles.length > 1"
                ref="galleryWrapper"
                z20
                w-full
                flex="~ justify-center"
                motion-safe="transition-opacity"
                :class="zoom.zoomedIn && 'opacity-0'"
            >
                <div
                    v-if="!isScrollable"
                    flex="~ items-center justify-center gap-0.5"
                    class="h-14 w-full"
                >
                    <div
                        v-for="(file, fIndex) in mediaFiles"
                        :id="`fileThumb-${file.id}`"
                        :key="file.id"
                        flex-shrink-0
                        cursor-pointer
                        motion-safe="transition-opacity"
                        :class="data.id === file.id ? 'opacity-100' : 'opacity-50'"
                        @click="handleGalleryItemClick(file)"
                    >
                        <img
                            v-if="file.mimeType!.startsWith('image/')"
                            :src="file.thumbnailUrl || file.directUrl"
                            :alt="file.fileName"
                            size-12
                            select-none
                            rounded-sm
                            object-cover
                            draggable="false"
                            loading="lazy"
                            :class="
                                fIndex === mediaFiles.indexOf(mediaFiles[0]!)
                                    ? 'rounded-l-lg'
                                    : fIndex === mediaFiles.length - 1 && 'rounded-r-lg'
                            "
                        />
                        <div
                            v-else
                            flex="~ items-center justify-center"
                            bg="white/10"
                            relative
                            size-12
                            select-none
                            rounded-sm
                            :class="
                                fIndex === mediaFiles.indexOf(mediaFiles[0]!)
                                    ? 'rounded-l-lg'
                                    : fIndex === mediaFiles.length - 1 && 'rounded-r-lg'
                            "
                        >
                            <Icon
                                :name="
                                    file.mimeType!.startsWith('audio/')
                                        ? 'heroicons-solid:musical-note'
                                        : 'heroicons-solid:video-camera'
                                "
                                size="24"
                                absolute
                                z11
                            />
                            <img
                                v-if="file.mimeType!.startsWith('video/')"
                                :src="file.thumbnailUrl!"
                                :alt="file.fileName"
                                absolute
                                z10
                                size-12
                                rounded-sm
                                object-cover
                                loading="lazy"
                                :class="
                                    fIndex === mediaFiles.indexOf(mediaFiles[0]!)
                                        ? 'rounded-l-lg'
                                        : fIndex === mediaFiles.length - 1 && 'rounded-r-lg'
                                "
                            />
                        </div>
                    </div>
                </div>

                <div
                    v-else
                    ref="scrollContainer"
                    relative
                    h-14
                    w-full
                    overflow-x-auto
                    scrollbar-none
                    @scroll.passive="onScroll"
                    @wheel.prevent="handleGalleryScroll"
                >
                    <div :style="{ width: `${totalVirtualWidth}px` }" relative h-full>
                        <div
                            v-for="item in virtualItems"
                            :id="`fileThumb-${item.file.id}`"
                            :key="item.file.id"
                            class="absolute top-1"
                            :style="{ left: `${item.left}px`, width: '48px' }"
                            cursor-pointer
                            motion-safe="transition-opacity"
                            :class="data.id === item.file.id ? 'opacity-100' : 'opacity-50'"
                            @click="handleGalleryItemClick(item.file)"
                        >
                            <img
                                v-if="item.file.mimeType!.startsWith('image/')"
                                :src="item.file.thumbnailUrl || item.file.directUrl"
                                :alt="item.file.fileName"
                                size-12
                                select-none
                                rounded-sm
                                object-cover
                                draggable="false"
                                loading="lazy"
                                :class="
                                    item.file.id === mediaFiles[0]!.id
                                        ? 'rounded-l-lg'
                                        : item.file.id === mediaFiles[mediaFiles.length - 1]!.id &&
                                          'rounded-r-lg'
                                "
                            />
                            <div
                                v-else
                                flex="~ items-center justify-center"
                                bg="white/10"
                                relative
                                size-12
                                select-none
                                rounded-sm
                                :class="
                                    item.file.id === mediaFiles[0]!.id
                                        ? 'rounded-l-lg'
                                        : item.file.id === mediaFiles[mediaFiles.length - 1]!.id &&
                                          'rounded-r-lg'
                                "
                            >
                                <Icon
                                    :name="
                                        item.file.mimeType!.startsWith('audio/')
                                            ? 'heroicons-solid:musical-note'
                                            : 'heroicons-solid:video-camera'
                                    "
                                    size="24"
                                    absolute
                                    z11
                                />
                                <img
                                    v-if="item.file.mimeType!.startsWith('video/')"
                                    :src="item.file.thumbnailUrl!"
                                    :alt="item.file.fileName"
                                    absolute
                                    z10
                                    size-12
                                    rounded-sm
                                    object-cover
                                    loading="lazy"
                                    :class="
                                        item.file.id === mediaFiles[0]!.id
                                            ? 'rounded-l-lg'
                                            : item.file.id ===
                                                  mediaFiles[mediaFiles.length - 1]!.id &&
                                              'rounded-r-lg'
                                    "
                                />
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </UiModal>
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';
import { Cubic, gsap } from 'gsap';

const { fileId: _fileId } = defineProps<{
    fileId: string;
    partial?: boolean;
}>();

const isOpen = defineModel<boolean>({ required: true });

const currentUser = useAuthUser();
const embed = useEmbed();
const { copied, copy } = useClipboard({ legacy: true });
const { $toast } = useNuxtApp();
const allFiles = useFiles();

const fileId = ref(_fileId);
const data = ref(allFiles.value.find((file) => file.id === fileId.value)!);

const isImage = computed(() => data.value.mimeType!.startsWith('image/'));
const isVideo = computed(() => data.value.mimeType!.startsWith('video/'));
const isAudio = computed(() => data.value.mimeType!.startsWith('audio/'));

const files = ref<FileData[]>([]);

const galleryWrapper = useTemplateRef('galleryWrapper');
const scrollContainer = useTemplateRef('scrollContainer');
const { width: containerWidth } = useElementSize(galleryWrapper);
const scrollLeft = ref(0);

const mediaFiles = computed(() =>
    files.value.filter((file) =>
        ['image/', 'video/', 'audio/'].some((type) => file.mimeType!.startsWith(type)),
    ),
);

const ITEM_SIZE = 50;
const OVERSCAN = 10;

const totalVirtualWidth = computed(() => mediaFiles.value.length * ITEM_SIZE);
const isScrollable = computed(() => {
    if (containerWidth.value === 0) return true;
    return totalVirtualWidth.value > containerWidth.value;
});

const onScroll = (e: Event) => {
    const target = e.target as HTMLElement;
    scrollLeft.value = target.scrollLeft;
};

const handleGalleryScroll = (e: WheelEvent) => {
    if (scrollContainer.value) scrollContainer.value.scrollLeft += e.deltaY;
};

const changeFile = async (newFile: FileData) => {
    await tl.value?.play();
    fileId.value = newFile.id;
    updateData();
    await tl.value?.reverse();
};

const handleGalleryItemClick = (newFile: FileData) => {
    if (fileId.value === newFile.id) return;
    changeFile(newFile);
};

const virtualItems = computed(() => {
    if (!isScrollable.value) return [];

    const startIndex = Math.max(0, Math.floor(scrollLeft.value / ITEM_SIZE) - OVERSCAN);
    const endIndex = Math.min(
        mediaFiles.value.length,
        Math.ceil((scrollLeft.value + (containerWidth.value || 0)) / ITEM_SIZE) + OVERSCAN,
    );

    const items = [];
    for (let i = startIndex; i < endIndex; i++) {
        items.push({
            file: mediaFiles.value[i]!,
            left: i * ITEM_SIZE,
        });
    }
    return items;
});

const scrollToActiveThumbnail = () => {
    if (!isScrollable.value || !scrollContainer.value) return;

    const index = mediaFiles.value.findIndex((f) => f.id === data.value.id);
    if (index === -1) return;

    const targetScroll = index * ITEM_SIZE - containerWidth.value / 2 + ITEM_SIZE / 2;

    scrollContainer.value.scrollTo({
        left: targetScroll,
        behavior: 'smooth',
    });
};

const modalId = useId();
const reducedMotion = usePreferredReducedMotion();

const canvas = useTemplateRef('canvasRef');
const video = useTemplateRef('videoRef');

const zoom = reactive({
    zoomedIn: false,
    originX: 'center',
    originY: 'center',
});

const index = computed(() => files.value.findIndex((file) => file.id === data.value.id));
const next = computed(() => {
    const file = files.value[index.value + 1];
    if (!file) return null;

    const allowedMimeTypes = ['image/', 'video/', 'audio/'];
    if (allowedMimeTypes.some((type) => file.mimeType!.startsWith(type))) return file;

    return files.value.find(
        (file, fIndex) =>
            allowedMimeTypes.some((type) => file.mimeType!.startsWith(type)) &&
            fIndex > index.value,
    );
});
const prev = computed(() => {
    const file = files.value[index.value - 1];
    if (!file) return null;

    const allowedMimeTypes = ['image/', 'video/', 'audio/'];
    if (allowedMimeTypes.some((type) => file.mimeType!.startsWith(type))) return file;

    return files.value.find(
        (file, fIndex) =>
            allowedMimeTypes.some((type) => file.mimeType!.startsWith(type)) &&
            fIndex < index.value,
    );
});
const isFirst = computed(() => index.value === 0);
const isLast = computed(() => index.value === files.value.length - 1);
const tl = computed(() =>
    reducedMotion.value === 'no-preference' && isOpen.value
        ? gsap.timeline({ paused: true, reversed: true }).to(`#${modalId}`, {
              opacity: 0,
              duration: 0.1,
              scale: 0.98,
              y: '2%',
              ease: Cubic.easeIn,
          })
        : null,
);

const deleting = ref(false);
const editModalOpen = ref(false);
const takingOut = ref(false);

const handleDelete = async () => {
    deleting.value = true;

    try {
        await $fetch(`/api/files/${data.value.id}`, { method: 'DELETE' });
        $toast.success('File deleted successfully');
    } catch (error: any) {
        $toast.error(error.data.message);
    }

    deleting.value = false;
};

const handleCopy = () => {
    copy(
        embed.value.enabled || data.value.embed?.enabled
            ? data.value.embedUrl!
            : data.value.directUrl!,
    );

    $toast.success('Link copied to clipboard');
};

const handleTakeOut = async () => {
    takingOut.value = true;

    try {
        await $fetch(`/api/files/${data.value.id}`, {
            method: 'PATCH',
            body: {
                folderId: null,
            },
        });
        $toast.success('File removed from folder successfully');
    } catch (error: any) {
        $toast.error(error.data.message);
    }

    takingOut.value = false;
};

const handlePrev = () => {
    if (!prev.value || !isOpen.value) return;
    changeFile(prev.value);
};

const handleNext = () => {
    if (!next.value || !isOpen.value) return;
    changeFile(next.value);
};

const handleAmbientMode = () => {
    if (!video.value || !canvas.value || reducedMotion.value !== 'no-preference' || !isOpen.value)
        return;

    const ctx = canvas.value!.getContext('2d', { alpha: false })!;
    let step: number | undefined;

    const setDimensions = () => {
        if (!video.value || !canvas.value) return;

        if (
            canvas.value.width !== video.value.videoWidth ||
            canvas.value.height !== video.value.videoHeight
        ) {
            canvas.value.width = video.value.videoWidth;
            canvas.value.height = video.value.videoHeight;
        }
    };

    const draw = () => {
        if (!canvas.value?.width || !canvas.value?.height) return;
        ctx.drawImage(video.value!, 0, 0, canvas.value!.width, canvas.value!.height);
    };

    const drawLoop = () => {
        draw();
        step = window.requestAnimationFrame(drawLoop);
    };

    const drawPause = () => {
        if (step) {
            window.cancelAnimationFrame(step);
            step = undefined;
        }
    };

    video.value.addEventListener('loadedmetadata', setDimensions, false);
    video.value.addEventListener(
        'loadeddata',
        () => {
            setDimensions();
            draw();
        },
        false,
    );
    video.value.addEventListener('seeked', draw, false);
    video.value.addEventListener('play', drawLoop, false);
    video.value.addEventListener('pause', drawPause, false);
    video.value.addEventListener('ended', drawPause, false);

    if (video.value.readyState >= 1) {
        setDimensions();
        draw();
    }
};

const handleZoom = (event: MouseEvent) => {
    if (!isImage.value) return;

    if (zoom.zoomedIn) {
        zoom.zoomedIn = false;
    } else {
        const target = event.target as HTMLImageElement;
        const rect = target.getBoundingClientRect();

        const x = event.clientX - rect.left;
        const y = event.clientY - rect.top;

        zoom.originX = `${x}px`;
        zoom.originY = `${y}px`;
        zoom.zoomedIn = true;
    }
};

const updateData = () => {
    files.value = data.value.folderId
        ? allFiles.value.filter((file) => file.folderId === data.value.folderId)
        : allFiles.value.filter((file) => !file.folderId);

    data.value = allFiles.value.find((file) => file.id === fileId.value)!;
};

onKeyStroke('ArrowLeft', handlePrev, { eventName: 'keydown' });
onKeyStroke('ArrowRight', handleNext, { eventName: 'keydown' });

watch(
    () => _fileId,
    (value) => {
        fileId.value = value;
        updateData();
    },
);

watch(
    allFiles,
    async (value) => {
        await nextTick();

        const updatedFile = value.find((file) => file.id === data.value.id);
        if (!updatedFile || data.value.folderId !== updatedFile.folderId) {
            const newFile = next.value || prev.value;
            if (newFile) return changeFile(newFile);

            return (isOpen.value = false);
        }

        updateData();
    },
    { deep: true },
);

watch([isOpen, editModalOpen], ([open, editModalOpen]) => {
    if (!open && !editModalOpen) {
        fileId.value = _fileId;
        zoom.zoomedIn = false;
    }

    if (open) {
        updateData();
        nextTick(scrollToActiveThumbnail);
    }
});

watch([isOpen, data], handleAmbientMode, { flush: 'post' });
watch(data, scrollToActiveThumbnail, { flush: 'post' });
</script>
