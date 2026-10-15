<template>
    <div
        class="fixed inset-0 overflow-hidden bg-black select-none"
        :style="{ cursor }"
        @pointerdown.capture="handlePointerDown"
        @pointermove="handlePointerMove"
        @pointerup="handlePointerUp"
        @pointercancel="handlePointerCancel"
        @lostpointercapture="handlePointerCancel"
        @pointerleave="handlePointerLeave"
        @contextmenu.prevent="handleRightClick"
    >
        <canvas ref="baseCanvas" class="absolute inset-0 size-full" />
        <div
            ref="annotationLayer"
            :class="cn('absolute inset-0', !hasCaptureToolbar && 'pointer-events-none')"
        />
        <canvas ref="drawCanvas" class="pointer-events-none absolute inset-0 size-full" />

        <div
            v-if="hasCaptureToolbar"
            ref="captureToolbar"
            class="absolute cursor-default"
            :style="captureToolbarStyle"
            @pointerdown.stop
            @pointermove.stop
            @pointerup.stop
            @contextmenu.stop.prevent
        >
            <div
                class="rounded-4xl border bg-popover p-1 text-popover-foreground shadow-md"
                :style="{ zoom: toolbarZoom }"
            >
                <AnnotationToolbar :annotator="annotator" :side="toolbarSide">
                    <template #start>
                        <UiButton
                            variant="ghost"
                            size="icon-sm"
                            aria-label="Move the toolbar"
                            class="cursor-grab touch-none active:cursor-grabbing"
                            @pointerdown="handleToolbarDragStart"
                            @pointermove="handleToolbarDragMove"
                            @pointerup="handleToolbarDragEnd"
                            @lostpointercapture="handleToolbarDragEnd"
                        >
                            <IGripHorizontal
                                v-if="toolbarSide === 'left' || toolbarSide === 'right'"
                            />
                            <IGripVertical v-else />
                        </UiButton>
                        <UiTooltip>
                            <UiTooltipTrigger as-child>
                                <UiButton
                                    :variant="!annotator.tool.value ? 'secondary' : 'ghost'"
                                    size="icon-sm"
                                    aria-label="Region"
                                    :aria-pressed="!annotator.tool.value"
                                    @click="annotator.setTool(null)"
                                >
                                    <IScan />
                                </UiButton>
                            </UiTooltipTrigger>
                            <UiTooltipContent :side="toolbarSide">
                                Region
                                <UiKbd class="ml-1">C</UiKbd>
                            </UiTooltipContent>
                        </UiTooltip>
                    </template>
                    <template #actions>
                        <UiTooltip>
                            <UiTooltipTrigger as-child>
                                <UiButton
                                    size="icon-sm"
                                    variant="ghost"
                                    aria-label="Cancel"
                                    @click="handleCancel"
                                >
                                    <IX />
                                </UiButton>
                            </UiTooltipTrigger>
                            <UiTooltipContent :side="toolbarSide">
                                Cancel
                                <UiKbd class="ml-1">Esc</UiKbd>
                            </UiTooltipContent>
                        </UiTooltip>
                    </template>
                </AnnotationToolbar>
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
import type { OverlayFinish, OverlayPayload, ScreenRect } from '../../shared/ipc';

type Point = { x: number; y: number };
type LocalRect = ScreenRect;
type LocalDisplay = { id: number; rect: LocalRect; scaleFactor: number; isPrimary: boolean };
type LocalWindow = LocalRect & { controls: LocalRect[] };
type Mode = 'idle' | 'pending' | 'drawing';
type ToolbarEdge = 'top' | 'bottom' | 'left' | 'right' | null;
type ToolbarPlacement = { display: number | null; edge: ToolbarEdge; cx: number; cy: number };

const DRAG_THRESHOLD = 3;
const MAGNIFIER_PIXELS = 15;
const MAGNIFIER_ZOOM = 8;
const HOVER_ANIMATION_MS = 140;
const TOOLBAR_MARGIN = 16;
const TOOLBAR_EDGE = 96;
const TOOLBAR_EDGE_Y = 64;
const TOOLBAR_MAGNET = 72;

const baseCanvas = useTemplateRef('baseCanvas');
const drawCanvas = useTemplateRef('drawCanvas');
const annotationLayer = useTemplateRef('annotationLayer');
const captureToolbar = useTemplateRef('captureToolbar');

const annotator = useAnnotator(annotationLayer);
const captureToolbarSize = useElementSize(captureToolbar, undefined, { box: 'border-box' });

const toolbarPlacement = useLocalStorage<ToolbarPlacement>(
    'fileship-capture-toolbar-placement',
    { display: null, edge: 'top', cx: 0.5, cy: 0 },
    { mergeDefaults: true },
);

const toolbarDrag = ref<{ id: number; offset: Point; pointer: Point; display: number } | null>(
    null,
);

const payload = shallowRef<OverlayPayload | null>(null);
const displays = shallowRef<LocalDisplay[]>([]);
const selection = ref<LocalRect | null>(null);
const mode = ref<Mode>('idle');
const cursor = ref('crosshair');

let windows: LocalWindow[] = [];
let pointer: Point | null = null;
let hover: LocalRect | null = null;
let hoverAnimation: { from: LocalRect; start: number } | null = null;
let regionDrag: { id: number; start: Point; isDeselect: boolean } | null = null;
let frame = 0;
let isFinishing = false;

const viewport = () => ({ x: 0, y: 0, width: window.innerWidth, height: window.innerHeight });

const ratio = () => (payload.value ? payload.value.bounds.width / window.innerWidth : 1);

const toLocal = (rect: ScreenRect): LocalRect => {
    const { bounds } = payload.value!;
    const scale = ratio();

    return {
        x: (rect.x - bounds.x) / scale,
        y: (rect.y - bounds.y) / scale,
        width: rect.width / scale,
        height: rect.height / scale,
    };
};

const contains = (rect: LocalRect, point: Point) =>
    point.x >= rect.x &&
    point.y >= rect.y &&
    point.x < rect.x + rect.width &&
    point.y < rect.y + rect.height;

const intersect = (rect: LocalRect, bounds: LocalRect): LocalRect | null => {
    const x = Math.max(rect.x, bounds.x);
    const y = Math.max(rect.y, bounds.y);
    const right = Math.min(rect.x + rect.width, bounds.x + bounds.width);
    const bottom = Math.min(rect.y + rect.height, bounds.y + bounds.height);

    return right > x && bottom > y ? { x, y, width: right - x, height: bottom - y } : null;
};

const fromPoints = (a: Point, b: Point): LocalRect => ({
    x: Math.min(a.x, b.x),
    y: Math.min(a.y, b.y),
    width: Math.abs(a.x - b.x),
    height: Math.abs(a.y - b.y),
});

const toPhysical = (rect: LocalRect) => {
    const scale = ratio();
    const x = Math.round(rect.x * scale);
    const y = Math.round(rect.y * scale);

    return {
        x,
        y,
        width: Math.max(1, Math.round((rect.x + rect.width) * scale) - x),
        height: Math.max(1, Math.round((rect.y + rect.height) * scale) - y),
    };
};

const toScreen = (rect: LocalRect): ScreenRect => {
    const { bounds } = payload.value!;
    const physical = toPhysical(rect);

    return { ...physical, x: physical.x + bounds.x, y: physical.y + bounds.y };
};

const displayAt = (point: Point) =>
    displays.value.find((item) => contains(item.rect, point)) ?? null;

const covers = (rect: LocalRect, inner: LocalRect) =>
    rect.x <= inner.x &&
    rect.y <= inner.y &&
    rect.x + rect.width >= inner.x + inner.width &&
    rect.y + rect.height >= inner.y + inner.height;

const targetAt = (point: Point) => {
    const display = displayAt(point)?.rect ?? viewport();
    const target = windows.find((item) => contains(item, point));
    if (!target) return display;

    const control = target.controls
        .filter((item) => contains(item, point))
        .sort((a, b) => a.width * a.height - b.width * b.height)[0];
    const rect = intersect(control ?? target, viewport());

    return !rect || covers(rect, display) ? display : rect;
};

const isRecording = computed(
    () => payload.value?.purpose === 'video' || payload.value?.purpose === 'gif',
);

const hasCaptureToolbar = computed(() => !!payload.value && payload.value.purpose === 'capture');

const isAnnotating = computed(() => hasCaptureToolbar.value && !!annotator.tool.value);

const toolbarDisplay = computed(() => {
    const id = toolbarDrag.value?.display ?? toolbarPlacement.value.display;

    return (
        displays.value.find((item) => item.id === id) ??
        displays.value.find((item) => item.isPrimary) ??
        displays.value[0] ??
        null
    );
});

const toolbarHost = computed(() => toolbarDisplay.value?.rect ?? viewport());

const toolbarZoom = computed(
    () => (toolbarDisplay.value?.scaleFactor ?? 1) / window.devicePixelRatio,
);

const edgeAt = (point: Point): ToolbarEdge => {
    const host = toolbarHost.value;

    if (point.x < host.x + TOOLBAR_EDGE) return 'left';
    if (point.x > host.x + host.width - TOOLBAR_EDGE) return 'right';
    if (point.y < host.y + TOOLBAR_EDGE_Y) return 'top';
    if (point.y > host.y + host.height - TOOLBAR_EDGE_Y) return 'bottom';
    return null;
};

const magnet = (value: number, middle: number) =>
    Math.abs(value - middle) < TOOLBAR_MAGNET ? middle : value;

const layoutToolbar = (edge: ToolbarEdge, center: Point) => {
    const host = toolbarHost.value;
    const width = captureToolbarSize.width.value;
    const height = captureToolbarSize.height.value;
    const minX = host.x + TOOLBAR_MARGIN;
    const minY = host.y + TOOLBAR_MARGIN;
    const maxX = host.x + host.width - TOOLBAR_MARGIN;
    const maxY = host.y + host.height - TOOLBAR_MARGIN;

    let x = center.x;
    let y = center.y;

    if (edge === 'left') x = minX + width / 2;
    if (edge === 'right') x = maxX - width / 2;
    if (edge === 'top') y = minY + height / 2;
    if (edge === 'bottom') y = maxY - height / 2;
    if (edge === 'left' || edge === 'right') y = magnet(y, host.y + host.height / 2);
    if (edge === 'top' || edge === 'bottom') x = magnet(x, host.x + host.width / 2);

    const left = clamp(x - width / 2, minX, Math.max(minX, maxX - width));
    const top = clamp(y - height / 2, minY, Math.max(minY, maxY - height));

    return { left, top, center: { x: left + width / 2, y: top + height / 2 } };
};

const currentEdge = computed(() =>
    toolbarDrag.value ? edgeAt(toolbarDrag.value.pointer) : toolbarPlacement.value.edge,
);

const currentLayout = computed(() => {
    const host = toolbarHost.value;
    const drag = toolbarDrag.value;
    const isOnEdge = currentEdge.value !== null;
    const center = drag
        ? {
              x: isOnEdge
                  ? drag.pointer.x
                  : drag.pointer.x - drag.offset.x + captureToolbarSize.width.value / 2,
              y: isOnEdge
                  ? drag.pointer.y
                  : drag.pointer.y - drag.offset.y + captureToolbarSize.height.value / 2,
          }
        : {
              x: host.x + toolbarPlacement.value.cx * host.width,
              y: host.y + toolbarPlacement.value.cy * host.height,
          };

    return layoutToolbar(currentEdge.value, center);
});

const toolbarSide = computed(() => {
    if (currentEdge.value === 'left') return 'right';
    if (currentEdge.value === 'right') return 'left';
    if (currentEdge.value === 'bottom') return 'top';
    return 'bottom';
});

const captureToolbarStyle = computed(() => ({
    left: `${currentLayout.value.left}px`,
    top: `${currentLayout.value.top}px`,
    visibility: captureToolbarSize.width.value ? ('visible' as const) : ('hidden' as const),
}));

const handleToolbarDragStart = (event: PointerEvent) => {
    const element = captureToolbar.value;
    if (!element || event.button !== 0) return;

    if (event.isTrusted) (event.currentTarget as HTMLElement).setPointerCapture(event.pointerId);

    const rect = element.getBoundingClientRect();

    toolbarDrag.value = {
        id: event.pointerId,
        offset: { x: event.clientX - rect.left, y: event.clientY - rect.top },
        pointer: { x: event.clientX, y: event.clientY },
        display: toolbarDisplay.value?.id ?? 0,
    };
};

const handleToolbarDragMove = (event: PointerEvent) => {
    const drag = toolbarDrag.value;
    if (!drag || event.pointerId !== drag.id) return;

    const point = pointFrom(event);

    toolbarDrag.value = { ...drag, pointer: point, display: displayAt(point)?.id ?? drag.display };
};

const handleToolbarDragEnd = (event: PointerEvent) => {
    const drag = toolbarDrag.value;
    if (!drag || event.pointerId !== drag.id) return;

    const host = toolbarHost.value;
    const { center } = currentLayout.value;

    toolbarPlacement.value = {
        display: toolbarDisplay.value?.id ?? null,
        edge: currentEdge.value,
        cx: (center.x - host.x) / host.width,
        cy: (center.y - host.y) / host.height,
    };
    toolbarDrag.value = null;
};

const isSameRect = (a: LocalRect | null, b: LocalRect | null) =>
    a === b ||
    (!!a && !!b && a.x === b.x && a.y === b.y && a.width === b.width && a.height === b.height);

const animatedHover = (): LocalRect | null => {
    if (!hoverAnimation || !hover) return hover;

    const progress = Math.min(1, (performance.now() - hoverAnimation.start) / HOVER_ANIMATION_MS);
    const eased = 1 - (1 - progress) ** 3;
    const { from } = hoverAnimation;
    const mix = (a: number, b: number) => a + (b - a) * eased;

    if (progress >= 1) {
        hoverAnimation = null;
        return hover;
    }

    scheduleRender();

    return {
        x: mix(from.x, hover.x),
        y: mix(from.y, hover.y),
        width: mix(from.width, hover.width),
        height: mix(from.height, hover.height),
    };
};

const setHover = (next: LocalRect | null) => {
    if (isSameRect(next, hover)) return;

    const from = animatedHover();
    hover = next;
    hoverAnimation = from && next ? { from, start: performance.now() } : null;
};

const themeColor = (name: string) =>
    getComputedStyle(document.documentElement).getPropertyValue(name).trim();

const drawLabel = (context: CanvasRenderingContext2D, text: string, x: number, y: number) => {
    context.font = `500 12px ${getComputedStyle(document.body).fontFamily}`;
    const width = context.measureText(text).width + 12;

    context.fillStyle = 'rgba(0, 0, 0, 0.75)';
    context.beginPath();
    context.roundRect(x, y, width, 20, 6);
    context.fill();

    context.fillStyle = '#ffffff';
    context.textBaseline = 'middle';
    context.fillText(text, x + 6, y + 10);

    return width;
};

const drawMagnifier = (context: CanvasRenderingContext2D, point: Point) => {
    const source = baseCanvas.value!;
    const bounds = displayAt(point)?.rect ?? viewport();
    const size = MAGNIFIER_PIXELS * MAGNIFIER_ZOOM;
    const physical = toPhysical({ x: point.x, y: point.y, width: 0, height: 0 });
    const half = Math.floor(MAGNIFIER_PIXELS / 2);

    let x = point.x + 24;
    let y = point.y + 24;
    if (x + size > bounds.x + bounds.width) x = point.x - size - 24;
    if (y + size + 28 > bounds.y + bounds.height) y = point.y - size - 52;

    context.save();
    context.beginPath();
    context.roundRect(x, y, size, size, 8);
    context.clip();
    context.fillStyle = '#000000';
    context.fillRect(x, y, size, size);
    context.imageSmoothingEnabled = false;
    context.drawImage(
        source,
        physical.x - half,
        physical.y - half,
        MAGNIFIER_PIXELS,
        MAGNIFIER_PIXELS,
        x,
        y,
        size,
        size,
    );

    context.strokeStyle = 'rgba(255, 255, 255, 0.12)';
    context.lineWidth = 1;
    context.beginPath();
    for (let i = 1; i < MAGNIFIER_PIXELS; i++) {
        context.moveTo(x + i * MAGNIFIER_ZOOM + 0.5, y);
        context.lineTo(x + i * MAGNIFIER_ZOOM + 0.5, y + size);
        context.moveTo(x, y + i * MAGNIFIER_ZOOM + 0.5);
        context.lineTo(x + size, y + i * MAGNIFIER_ZOOM + 0.5);
    }
    context.stroke();

    context.strokeStyle = themeColor('--primary');
    context.lineWidth = 2;
    context.strokeRect(
        x + half * MAGNIFIER_ZOOM,
        y + half * MAGNIFIER_ZOOM,
        MAGNIFIER_ZOOM,
        MAGNIFIER_ZOOM,
    );
    context.restore();

    context.strokeStyle = 'rgba(255, 255, 255, 0.8)';
    context.lineWidth = 1;
    context.beginPath();
    context.roundRect(x - 0.5, y - 0.5, size + 1, size + 1, 8);
    context.stroke();

    const { bounds: origin } = payload.value!;
    drawLabel(context, `${physical.x + origin.x}, ${physical.y + origin.y}`, x, y + size + 6);
};

const render = () => {
    frame = 0;

    const canvas = drawCanvas.value;
    if (!canvas || !payload.value) return;

    const context = canvas.getContext('2d')!;
    const bounds = viewport();
    const dpr = window.devicePixelRatio;
    const primary = themeColor('--primary');
    const region = mode.value === 'drawing' ? selection.value : animatedHover();

    context.setTransform(dpr, 0, 0, dpr, 0, 0);
    context.clearRect(0, 0, bounds.width, bounds.height);

    if (isAnnotating.value) return;

    context.fillStyle = 'rgba(0, 0, 0, 0.45)';
    context.fillRect(0, 0, bounds.width, bounds.height);

    if (region) {
        context.clearRect(region.x, region.y, region.width, region.height);

        context.strokeStyle = primary;
        context.lineWidth = mode.value === 'drawing' ? 2 : 1.5;
        context.setLineDash(mode.value === 'drawing' ? [] : [6, 4]);
        context.strokeRect(region.x, region.y, region.width, region.height);
        context.setLineDash([]);
    }

    if (mode.value === 'drawing' && selection.value) {
        const rect = selection.value;
        const physical = toPhysical(rect);
        const labelY = rect.y >= 28 ? rect.y - 26 : rect.y + 6;

        drawLabel(context, `${physical.width} × ${physical.height}`, rect.x, labelY);
    }

    if (payload.value.showMagnifier && pointer) drawMagnifier(context, pointer);
};

const scheduleRender = () => {
    if (!frame) frame = requestAnimationFrame(render);
};

const resizeDrawCanvas = () => {
    const canvas = drawCanvas.value;
    if (!canvas || !payload.value) return;

    canvas.width = Math.round(window.innerWidth * window.devicePixelRatio);
    canvas.height = Math.round(window.innerHeight * window.devicePixelRatio);
    scheduleRender();
};

const reset = () => {
    selection.value = null;
    mode.value = 'idle';
    cursor.value = 'crosshair';
    pointer = null;
    hover = null;
    hoverAnimation = null;
    regionDrag = null;
    annotator.reset();
    annotator.tool.value = null;
};

const finish = (result: OverlayFinish) => {
    if (isFinishing) return;
    isFinishing = true;

    desktop('overlay:finish', result)
        .catch(() => desktop('overlay:finish', { type: 'cancel' }))
        .finally(() => {
            isFinishing = false;
        });
};

const finishRegion = (rect: LocalRect) => {
    const screenRect = toScreen(rect);
    if (isRecording.value) return finish({ type: 'area', rect: screenRect });

    const canvas = annotator.renderRegion(toLocal(screenRect));
    const { data } = canvas.getContext('2d')!.getImageData(0, 0, canvas.width, canvas.height);

    finish({
        type: 'region',
        rect: screenRect,
        width: canvas.width,
        height: canvas.height,
        rgba: new Uint8Array(data.buffer),
    });
};

const pointFrom = (event: MouseEvent): Point => ({ x: event.clientX, y: event.clientY });

const isLargeEnough = (rect: LocalRect) => {
    const physical = toPhysical(rect);
    return physical.width >= 2 && physical.height >= 2;
};

const cancelRegion = () => {
    regionDrag = null;
    selection.value = null;
    mode.value = 'idle';
    if (pointer) setHover(targetAt(pointer));
    scheduleRender();
};

const isOnToolbar = (event: Event) => !!captureToolbar.value?.contains(event.target as Node);

const handlePointerDown = (event: PointerEvent) => {
    if (event.button !== 0 || !payload.value || isAnnotating.value || regionDrag) return;
    if (annotator.isEditingText.value || isOnToolbar(event)) return;

    const point = pointFrom(event);
    if (annotator.isOverShape(point)) return;

    if (event.isTrusted) (event.currentTarget as HTMLElement).setPointerCapture(event.pointerId);

    regionDrag = { id: event.pointerId, start: point, isDeselect: !!annotator.selectedId.value };
    selection.value = null;
    mode.value = 'pending';
    scheduleRender();
};

const handlePointerMove = (event: PointerEvent) => {
    if (!payload.value) return;

    const point = pointFrom(event);
    pointer = point;

    const drag = regionDrag;

    if (drag && event.pointerId === drag.id) {
        const distance = Math.hypot(point.x - drag.start.x, point.y - drag.start.y);
        if (mode.value === 'pending' && distance >= DRAG_THRESHOLD) mode.value = 'drawing';

        if (mode.value === 'drawing') {
            let end = point;

            if (event.shiftKey) {
                const size = Math.max(
                    Math.abs(point.x - drag.start.x),
                    Math.abs(point.y - drag.start.y),
                );
                end = {
                    x: drag.start.x + Math.sign(point.x - drag.start.x) * size,
                    y: drag.start.y + Math.sign(point.y - drag.start.y) * size,
                };
            }

            selection.value = intersect(fromPoints(drag.start, end), viewport());
        }
    } else if (!drag) {
        setHover(targetAt(point));
    }

    cursor.value =
        !drag && !isAnnotating.value && annotator.isOverShape(point) ? 'move' : 'crosshair';
    scheduleRender();
};

const handlePointerUp = (event: PointerEvent) => {
    const drag = regionDrag;
    if (!drag || event.pointerId !== drag.id || !payload.value) return;

    const isClick = mode.value === 'pending';
    const rect = isClick ? targetAt(pointFrom(event)) : selection.value;
    regionDrag = null;

    if (isClick && drag.isDeselect) return cancelRegion();

    if (rect && isLargeEnough(rect)) return finishRegion(rect);
    cancelRegion();
};

const handlePointerCancel = (event: PointerEvent) => {
    if (regionDrag && event.pointerId === regionDrag.id) cancelRegion();
};

const handlePointerLeave = () => {
    if (regionDrag) return;

    pointer = null;
    setHover(null);
    scheduleRender();
};

const handleRightClick = (event: MouseEvent) => {
    if (regionDrag) return cancelRegion();
    if (annotator.isEditingText.value || isOnToolbar(event)) return;
    if (annotator.selectedId.value) return annotator.select(null);
    if (annotator.tool.value) return annotator.setTool(null);

    finish({ type: 'cancel' });
};

const handleCancel = () => finish({ type: 'cancel' });

const handleKeyDown = (event: KeyboardEvent) => {
    if (!payload.value || annotator.isEditingText.value) return;

    if (event.key === 'Escape') {
        if (regionDrag) return cancelRegion();
        if (annotator.selectedId.value) return annotator.select(null);
        return finish({ type: 'cancel' });
    }

    if (hasCaptureToolbar.value) {
        const isPlain = !event.ctrlKey && !event.altKey && !event.metaKey;

        if (isPlain && event.key.toLowerCase() === 'c') {
            event.preventDefault();
            return annotator.setTool(null);
        }

        if (annotator.handleKey(event)) {
            event.preventDefault();
            return;
        }
    }

    if (regionDrag) return;

    if (event.key === ' ') return finishRegion(viewport());

    if (event.key === 'Enter') {
        const rect = pointer ? targetAt(pointer) : hover;
        if (rect) finishRegion(rect);
        return;
    }

    const display = /^[1-9]$/.test(event.key) ? displays.value[Number(event.key) - 1] : null;
    if (display) finishRegion(display.rect);
};

const toImageData = (pixels: Uint8Array, width: number, height: number) => {
    const bytes = pixels.byteOffset % 4 ? pixels.slice() : pixels;
    const source = new Uint32Array(bytes.buffer, bytes.byteOffset, bytes.length / 4);
    const rgba = new Uint8ClampedArray(source.length * 4);
    const target = new Uint32Array(rgba.buffer);

    for (let i = 0; i < source.length; i++) {
        const pixel = source[i]!;
        target[i] = (pixel & 0xff00ff00) | ((pixel & 0xff) << 16) | ((pixel >>> 16) & 0xff);
    }

    return new ImageData(rgba, width, height);
};

const nextFrame = () => new Promise((resolve) => requestAnimationFrame(resolve));

const handleOpen = async (value: OverlayPayload) => {
    reset();
    isFinishing = false;
    payload.value = value;

    const { bounds } = value;
    const canvas = baseCanvas.value!;
    canvas.width = bounds.width;
    canvas.height = bounds.height;

    const context = canvas.getContext('2d')!;
    context.fillStyle = '#000000';
    context.fillRect(0, 0, bounds.width, bounds.height);

    for (const display of value.displays) {
        context.putImageData(
            toImageData(display.pixels, display.rect.width, display.rect.height),
            display.rect.x - bounds.x,
            display.rect.y - bounds.y,
        );
    }

    displays.value = value.displays.map((item) => ({
        id: item.id,
        rect: toLocal(item.rect),
        scaleFactor: item.scaleFactor,
        isPrimary: item.isPrimary,
    }));
    windows = value.windows.map((item) => ({
        ...toLocal(item),
        controls: item.controls.map(toLocal),
    }));

    const cursorPoint = toLocal({ ...value.cursor, width: 0, height: 0 });
    pointer = { x: cursorPoint.x, y: cursorPoint.y };
    hover = targetAt(pointer);

    annotator.resize(window.innerWidth, window.innerHeight);
    annotator.setSource(canvas, ratio());
    resizeDrawCanvas();
    render();

    await nextFrame();
    await nextFrame();
    desktop('overlay:ready');
};

const handleClose = () => {
    payload.value = null;
    displays.value = [];
    windows = [];
    reset();
    annotator.unmount();

    for (const canvas of [baseCanvas.value, drawCanvas.value]) {
        if (!canvas) continue;
        canvas.width = 0;
        canvas.height = 0;
    }
};

useHead({
    style: [{ innerHTML: '#nuxt-devtools-container { display: none !important; }' }],
});

useEventListener('keydown', handleKeyDown);
useEventListener('resize', resizeDrawCanvas);

onMounted(() => {
    annotator.tool.value = null;

    const offOpen = window.fileship.on('overlay:open', handleOpen);
    const offClose = window.fileship.on('overlay:close', handleClose);

    onBeforeUnmount(() => {
        offOpen();
        offClose();
    });

    desktop('overlay:loaded');
});

definePageMeta({
    layout: false,
});
</script>
