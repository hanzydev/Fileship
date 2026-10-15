<template>
    <Head>
        <Title>Edit</Title>
    </Head>

    <div class="flex h-dvh flex-col bg-background text-foreground">
        <header class="flex flex-wrap items-center gap-2 border-b px-3 py-2">
            <AnnotationToolbar :annotator="annotator" />

            <div class="ml-auto flex shrink-0 items-center gap-1">
                <UiButton
                    variant="ghost"
                    size="icon-sm"
                    aria-label="Zoom out"
                    @click="handleZoom(-1)"
                >
                    <IZoomOut />
                </UiButton>
                <UiButton
                    variant="ghost"
                    size="sm"
                    class="min-w-16 tabular-nums"
                    @click="handleFit"
                >
                    {{ Math.round(zoom * 100) }}%
                </UiButton>
                <UiButton
                    variant="ghost"
                    size="icon-sm"
                    aria-label="Zoom in"
                    @click="handleZoom(1)"
                >
                    <IZoomIn />
                </UiButton>

                <UiSeparator orientation="vertical" class="mx-1 h-6" />

                <UiButton
                    variant="ghost"
                    size="sm"
                    :disabled="!image"
                    @click="handleFinish('cancel')"
                >
                    Cancel
                </UiButton>
                <UiButton
                    variant="outline"
                    size="sm"
                    :disabled="!image"
                    @click="handleFinish('copy')"
                >
                    <ICopy />
                    Copy
                </UiButton>
                <UiButton
                    variant="outline"
                    size="sm"
                    :disabled="!image"
                    @click="handleFinish('save')"
                >
                    <ISave />
                    Save as
                </UiButton>
                <UiButton size="sm" :disabled="!image" @click="handleFinish('continue')">
                    <IUpload v-if="image?.continueLabel === 'Upload'" />
                    <ICheck v-else />
                    {{ image?.continueLabel ?? 'Done' }}
                </UiButton>
            </div>
        </header>

        <main ref="viewport" class="flex flex-1 overflow-auto bg-muted" @wheel="handleWheel">
            <div class="m-auto p-8">
                <div
                    class="relative shadow-lg"
                    :style="{ width: `${size.width * zoom}px`, height: `${size.height * zoom}px` }"
                >
                    <canvas ref="imageCanvas" class="absolute inset-0 size-full" />
                    <div ref="annotationLayer" class="absolute inset-0" />
                </div>
            </div>
        </main>
    </div>
</template>

<script setup lang="ts">
import type { EditorFinish, EditorImage } from '../../shared/ipc';

const ZOOM_STEPS = [0.1, 0.25, 0.33, 0.5, 0.67, 0.75, 1, 1.25, 1.5, 2, 3, 4];

const route = useRoute();
const { $toast } = useNuxtApp();

const id = String(route.query.id ?? '');
const viewport = useTemplateRef('viewport');
const imageCanvas = useTemplateRef('imageCanvas');
const annotationLayer = useTemplateRef('annotationLayer');

const annotator = useAnnotator(annotationLayer);
const image = shallowRef<EditorImage | null>(null);
const size = reactive({ width: 0, height: 0 });
const zoom = ref(1);
let isFinishing = false;

const setZoom = (value: number) => {
    zoom.value = clamp(value, ZOOM_STEPS[0]!, ZOOM_STEPS.at(-1)!);
    annotator.resize(size.width, size.height, zoom.value);
};

const fitZoom = () => {
    const element = viewport.value;
    if (!element || !size.width) return 1;

    return Math.min(
        1,
        (element.clientWidth - 64) / size.width,
        (element.clientHeight - 64) / size.height,
    );
};

const handleFit = () => setZoom(fitZoom());

const handleZoom = (direction: 1 | -1) => {
    const next =
        direction > 0
            ? ZOOM_STEPS.find((step) => step > zoom.value + 0.001)
            : ZOOM_STEPS.findLast((step) => step < zoom.value - 0.001);

    setZoom(next ?? zoom.value);
};

const handleWheel = (event: WheelEvent) => {
    if (!event.ctrlKey) return;

    event.preventDefault();
    handleZoom(event.deltaY < 0 ? 1 : -1);
};

const handleFinish = async (action: EditorFinish['action']) => {
    if (isFinishing || !image.value) return;
    isFinishing = true;

    try {
        if (action === 'cancel') {
            await desktop('editor:finish', id, { action });
            return;
        }

        const canvas = annotator.renderRegion({
            x: 0,
            y: 0,
            width: size.width,
            height: size.height,
        });
        const { data } = canvas.getContext('2d')!.getImageData(0, 0, canvas.width, canvas.height);

        await desktop('editor:finish', id, {
            action,
            width: canvas.width,
            height: canvas.height,
            rgba: new Uint8Array(data.buffer),
        });
    } catch (error: any) {
        isFinishing = false;
        $toast.error(error.message);
    }
};

const handleKeyDown = (event: KeyboardEvent) => {
    if (annotator.isEditingText.value) return;

    if (event.key === 'Escape') {
        annotator.select(null);
        return;
    }

    if (event.ctrlKey && event.key.toLowerCase() === 's') {
        event.preventDefault();
        handleFinish('save');
        return;
    }

    if (event.ctrlKey && event.key.toLowerCase() === 'c' && !annotator.selectedId.value) {
        event.preventDefault();
        handleFinish('copy');
        return;
    }

    if (event.ctrlKey && event.key === 'Enter') {
        handleFinish('continue');
        return;
    }

    if (event.ctrlKey && (event.key === '=' || event.key === '+')) return handleZoom(1);
    if (event.ctrlKey && event.key === '-') return handleZoom(-1);
    if (event.ctrlKey && event.key === '0') return handleFit();

    if (annotator.handleKey(event)) event.preventDefault();
};

useEventListener('keydown', handleKeyDown);

onMounted(async () => {
    try {
        image.value = await desktop('editor:get', id);
    } catch (error: any) {
        $toast.error(error.message);
        return;
    }

    const bitmap = await createImageBitmap(
        new Blob([image.value.png as BlobPart], { type: 'image/png' }),
    );
    const canvas = imageCanvas.value!;

    size.width = bitmap.width;
    size.height = bitmap.height;
    canvas.width = bitmap.width;
    canvas.height = bitmap.height;
    canvas.getContext('2d')!.drawImage(bitmap, 0, 0);
    bitmap.close();

    await nextTick();

    zoom.value = fitZoom();
    annotator.mount(size.width, size.height, zoom.value);
    annotator.setSource(canvas, 1);
    annotator.tool.value = 'rectangle';
});

definePageMeta({
    layout: false,
});
</script>
