<template>
    <div ref="scroller" class="size-full overflow-y-auto overscroll-contain">
        <div class="mx-auto w-full max-w-3xl">
            <UiSkeleton v-if="!pdf" class="w-full rounded-md" :style="{ aspectRatio: ratio }" />
            <div
                v-else
                class="relative w-full"
                :style="{ height: `${virtualizer.getTotalSize()}px` }"
            >
                <div
                    v-for="row in virtualizer.getVirtualItems()"
                    :key="row.index"
                    :ref="(el) => virtualizer.measureElement(el as Element)"
                    :data-index="row.index"
                    class="absolute top-0 left-0 w-full pb-4"
                    :style="{ transform: `translateY(${row.start}px)` }"
                >
                    <canvas
                        :ref="(el) => el && drawPage(row.index, el as HTMLCanvasElement)"
                        role="img"
                        :aria-label="$t('fileViewer.page', { page: row.index + 1 })"
                        class="w-full rounded-md bg-muted"
                        :style="{ aspectRatio: ratios[row.index] ?? ratio }"
                    />
                </div>
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
import type {
    PDFDocumentLoadingTask,
    PDFDocumentProxy,
    RenderTask,
} from 'pdfjs-dist/legacy/build/pdf.mjs';

import { useVirtualizer } from '@tanstack/vue-virtual';

const { src, range = true } = defineProps<{
    src: string;
    range?: boolean;
}>();

const emit = defineEmits<{
    error: [];
}>();

const MAX_PAGE_WIDTH = 768;
const MAX_PIXEL_RATIO = 2;
const PAGE_GAP = 16;

const scroller = useTemplateRef('scroller');
const { width } = useElementSize(scroller);

const pdf = shallowRef<PDFDocumentProxy>();
const aspect = ref(Math.SQRT2);
const ratio = ref(`1 / ${Math.SQRT2}`);
const ratios = ref<Record<number, string>>({});
const drawn = new WeakMap<HTMLCanvasElement, number>();
const tasks = new WeakMap<HTMLCanvasElement, RenderTask>();

let loadingTask: PDFDocumentLoadingTask | undefined;
let version = 0;

const virtualizer = useVirtualizer(
    computed(() => ({
        count: pdf.value?.numPages ?? 0,
        estimateSize: () => Math.min(width.value, MAX_PAGE_WIDTH) * aspect.value + PAGE_GAP,
        overscan: 2,
        getScrollElement: () => scroller.value,
    })),
);

const drawPage = async (index: number, canvas: HTMLCanvasElement) => {
    const cssWidth = canvas.clientWidth;
    if (!pdf.value || !cssWidth || drawn.get(canvas) === cssWidth) return;

    drawn.set(canvas, cssWidth);

    const page = await pdf.value.getPage(index + 1);
    if (drawn.get(canvas) !== cssWidth) return;

    const viewport = page.getViewport({ scale: 1 });
    const scaled = page.getViewport({
        scale: (cssWidth * Math.min(devicePixelRatio, MAX_PIXEL_RATIO)) / viewport.width,
    });

    ratios.value[index] = `${viewport.width} / ${viewport.height}`;
    tasks.get(canvas)?.cancel();

    canvas.width = Math.floor(scaled.width);
    canvas.height = Math.floor(scaled.height);

    const task = page.render({ canvas, viewport: scaled });
    tasks.set(canvas, task);

    await task.promise.catch(() => null);
};

const load = async (url: string) => {
    const current = ++version;

    loadingTask?.destroy();
    loadingTask = undefined;
    pdf.value = undefined;
    ratios.value = {};
    scroller.value?.scrollTo({ top: 0 });

    try {
        const task = await openPdf(url, range);
        if (current !== version) return task.destroy();

        loadingTask = task;

        const document = await task.promise;
        const { width, height } = (await document.getPage(1)).getViewport({ scale: 1 });
        if (current !== version) return;

        aspect.value = height / width;
        ratio.value = `${width} / ${height}`;
        pdf.value = document;

        await nextTick();
        if (current === version) virtualizer.value.measure();
    } catch {
        if (current === version) emit('error');
    }
};

watch(() => src, load);

onMounted(() => load(src));

onBeforeUnmount(() => {
    version++;
    loadingTask?.destroy();
});
</script>
