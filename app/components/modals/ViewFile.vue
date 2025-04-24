<template>
    <ModalsEditFile v-if="currentUser?.id === data?.authorId" v-model="editModalOpen" :data />

    <Teleport to="body">
        <Transition
            enter-active-class="motion-safe:(animate-in fade-in zoom-in-95 slide-in-top-48%)"
            leave-active-class="motion-safe:(animate-out fade-out zoom-out-95 slide-out-top-48%)"
        >
            <UiButton
                v-if="isOpen && files.length > 1"
                variant="secondary"
                alignment="center"
                icon="heroicons-solid:arrow-narrow-left"
                icon-class="text-2xl! sm:text-3xl!"
                rounded-full="!"
                p0="!"
                shadow="xl fs-overlay-1"
                aria-label="Previous"
                top="1/2"
                translate-y-="1/2"
                data-controller
                absolute
                sm="left-6"
                left-3
                z60
                size="10 sm:14"
                :disabled="isFirst || !prev"
                @click="handlePrev"
            />
        </Transition>
        <Transition
            enter-active-class="motion-safe:(animate-in fade-in zoom-in-95 slide-in-top-48%)"
            leave-active-class="motion-safe:(animate-out fade-out zoom-out-95 slide-out-top-48%)"
        >
            <UiButton
                v-if="isOpen && files.length > 1"
                variant="secondary"
                alignment="center"
                icon="heroicons-solid:arrow-narrow-right"
                icon-class="text-2xl! sm:text-3xl!"
                rounded-full="!"
                p0="!"
                shadow="xl fs-overlay-1"
                aria-label="Previous"
                top="1/2"
                translate-y-="1/2"
                data-controller
                absolute
                sm="right-6"
                right-3
                z60
                size="10 sm:14"
                :disabled="isLast || !next"
                @click="handleNext"
            />
        </Transition>
    </Teleport>

    <UiModal
        v-if="isImage || isVideo || isAudio"
        :id="modalId"
        v-model="isOpen"
        max-w768px
        p8
        space-y-4
        background-class="z50!"
    >
        <div flex="~ justify-between" wfull>
            <h2 line-clamp-2 break-all>{{ data.fileName }}</h2>

            <div flex="~ gap2.5">
                <UiButton
                    v-if="currentUser?.id === data.authorId"
                    alignment="center"
                    class="h10 w10 !p0 hover:text-white"
                    icon="heroicons:pencil-16-solid"
                    icon-size="24"
                    ring="1 fs-overlay-4"
                    @click="
                        isOpen = false;
                        editModalOpen = true;
                    "
                />

                <UiButton
                    variant="accent"
                    alignment="center"
                    class="h10 w10 !p0 hover:text-white"
                    icon="heroicons-solid:x"
                    icon-size="24"
                    @click="isOpen = false"
                />
            </div>
        </div>

        <div grid="~ gap4" :class="data.views ? 'sm:grid-cols-3' : 'sm:grid-cols-2'">
            <div v-if="data.views" flex="~ items-center gap2" rounded-lg bg-fs-overlay-2 p4>
                <Icon name="heroicons-solid:eye" text-fs-accent size="24" />
                <span font-medium="!">{{ data.views.today }} today</span>
            </div>
            <div flex="~ items-center gap2" rounded-lg bg-fs-overlay-2 p4>
                <Icon name="mdi:sd-storage" text-fs-accent size="24" />
                <span font-medium="!">{{ data.size!.formatted }}</span>
            </div>
            <div flex="~ items-center gap2" rounded-lg bg-fs-overlay-2 p4>
                <Icon name="heroicons-solid:calendar" text-fs-accent size="24" />
                <span font-medium="!">
                    {{ dayjs(data.createdAt).fromNow() }}
                </span>
            </div>
        </div>

        <div v-if="isImage" flex="~ items-center justify-center" relative wfull>
            <div
                v-if="reducedMotion === 'no-preference'"
                flex="~ items-center justify-center'"
                absolute
                z10
                size-full
                h96
                wfull
                cursor-pointer
                select-none
                rounded-lg
                bg-contain
                bg-center
                bg-no-repeat
                blur-3xl
                :style="{
                    backgroundImage: `url(${data.directUrl})`,
                }"
            />
            <div
                z11
                h96
                wfull
                cursor-pointer
                select-none
                rounded-lg
                bg-contain
                bg-center
                bg-no-repeat
                :style="{
                    backgroundImage: `url(${data.directUrl})`,
                }"
                @click="handleFullScreen"
            />
        </div>

        <div v-else-if="isVideo" flex="~ items-center justify-center" relative wfull>
            <canvas ref="canvasRef" absolute z10 size-full blur-3xl />
            <video
                ref="videoRef"
                :src="data.directUrl"
                :poster="data.thumbnailUrl !== null ? data.thumbnailUrl : undefined"
                controls
                relative
                z11
                h96
                wfull
                rounded-lg
            />
        </div>

        <audio v-else-if="isAudio" :src="data.directUrl" controls wfull />

        <div flex="~ gap4 <md:col">
            <UiButton
                icon="heroicons-solid:download"
                icon-size="24"
                wfull
                gap2
                :href="`${data.directUrl}?download`"
                target="_blank"
            >
                Download
            </UiButton>

            <UiButton
                :icon="
                    copied ? 'heroicons-solid:clipboard-check' : 'heroicons-solid:clipboard-copy'
                "
                :icon-class="copied && 'text-green500'"
                icon-size="20"
                wfull
                gap2
                @click="handleCopy"
            >
                Copy Link
            </UiButton>

            <UiButton
                v-if="currentUser?.id === data.authorId"
                variant="dangerFill"
                icon="heroicons-solid:trash"
                icon-size="20"
                wfull
                gap2
                :disabled="deleting"
                :loading="deleting"
                @click="handleDelete"
            >
                Delete
            </UiButton>
        </div>
    </UiModal>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';
import { Cubic, gsap } from 'gsap';
import { toast } from 'vue-sonner';

const { data: _data } = defineProps<{
    data: Partial<FileData> & { embed?: IEmbed };
}>();

const isOpen = defineModel<boolean>({ required: true });

const data = ref(_data);

const isImage = computed(() => data.value.mimeType!.startsWith('image/'));
const isVideo = computed(() => data.value.mimeType!.startsWith('video/'));
const isAudio = computed(() => data.value.mimeType!.startsWith('audio/'));

const folders = useFolders();
const currentUser = useAuthUser();
const embed = useEmbed();
const { copied, copy } = useClipboard({ legacy: true });

const allFiles = useFiles();
const files = computed(() =>
    data.value.folderId
        ? folders.value
              .find((folder) => folder.id === data.value.folderId)!
              .files.map((fileId) => allFiles.value.find((file) => file.id === fileId)!)
        : allFiles.value.filter((file) => !file.folderId),
);

const modalId = useId();
const reducedMotion = usePreferredReducedMotion();

const canvas = useTemplateRef('canvasRef');
const video = useTemplateRef('videoRef');

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
              duration: 0.15,
              scale: 0.95,
              y: '2%',
              ease: Cubic.easeIn,
          })
        : null,
);

const deleting = ref(false);
const editModalOpen = ref(false);

const handleDelete = async () => {
    deleting.value = true;
    await $fetch(`/api/files/${data.value.id}`, { method: 'DELETE' });
    deleting.value = false;

    toast.success('File deleted successfully');
};

const handleCopy = () => {
    copy(
        embed.value.enabled || data.value.embed?.enabled
            ? data.value.embedUrl!
            : data.value.directUrl!,
    );

    toast.success('Link copied to clipboard');
};

const handleFullScreen = (event: MouseEvent) => {
    const target = event.target as HTMLImageElement;

    if (document.fullscreenElement) {
        document.exitFullscreen();
    } else {
        target.requestFullscreen();
    }
};

const handlePrev = async () => {
    if (!prev.value) return;

    await tl.value?.play();
    data.value = prev.value!;
    await tl.value?.reverse();
};

const handleNext = async () => {
    if (!next.value) return;

    await tl.value?.play();
    data.value = next.value!;
    await tl.value?.reverse();
};

const handleAmbientMode = () => {
    if (!video.value || !canvas.value || reducedMotion.value !== 'no-preference' || !isOpen.value)
        return;

    const ctx = canvas.value!.getContext('2d')!;
    let step: number | undefined;

    const draw = () => {
        ctx.drawImage(video.value!, 0, 0, canvas.value!.width, canvas.value!.height);
    };

    const drawLoop = () => {
        draw();
        step = window.requestAnimationFrame(drawLoop);
    };

    const drawPause = () => {
        window.cancelAnimationFrame(step!);
        step = undefined;
    };

    video.value.addEventListener('loadeddata', draw, false);
    video.value.addEventListener('seeked', draw, false);
    video.value.addEventListener('play', drawLoop, false);
    video.value.addEventListener('pause', drawPause, false);
    video.value.addEventListener('ended', drawPause, false);
};

onKeyStroke('ArrowLeft', handlePrev, { eventName: 'keydown' });
onKeyStroke('ArrowRight', handleNext, { eventName: 'keydown' });

watch(
    () => _data,
    (value) => {
        if (value.id === data.value.id) data.value = value;
    },
);

watch(
    files,
    (value) => {
        if (!value.find((file) => file.id === data.value.id)) isOpen.value = false;
    },
    { immediate: true },
);

watch([isOpen, editModalOpen], ([open, editModalOpen]) => {
    if (!open && !editModalOpen) data.value = _data;
});

watch([isOpen, data], handleAmbientMode, { flush: 'post' });
</script>
