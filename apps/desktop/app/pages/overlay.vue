<template>
    <div
        class="fixed inset-0 overflow-hidden bg-black select-none"
        :style="{ cursor }"
        @mousedown="handleMouseDown"
        @mousemove="handleMouseMove"
        @mouseup="handleMouseUp"
        @mouseleave="handleMouseLeave"
        @dblclick="handleDoubleClick"
        @contextmenu.prevent="handleRightClick"
    >
        <canvas ref="baseCanvas" class="absolute inset-0 size-full" />
        <div
            ref="annotationLayer"
            :class="cn('absolute inset-0', !isAnnotating && 'pointer-events-none')"
        />
        <canvas ref="drawCanvas" class="pointer-events-none absolute inset-0 size-full" />

        <div
            v-if="hasCaptureToolbar"
            ref="captureToolbar"
            class="absolute rounded-4xl border bg-popover p-1 text-popover-foreground shadow-md"
            :style="captureToolbarStyle"
            @mousedown.stop
            @mousemove.stop
            @mouseup.stop
            @dblclick.stop
            @contextmenu.stop.prevent
        >
            <AnnotationToolbar :annotator="annotator" :side="toolbarSide" :disabled="!selection">
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
                        <IGripHorizontal v-if="toolbarSide === 'left' || toolbarSide === 'right'" />
                        <IGripVertical v-else />
                    </UiButton>
                    <UiButton
                        :variant="!annotator.tool.value ? 'secondary' : 'ghost'"
                        size="icon-sm"
                        aria-label="Adjust area"
                        :aria-pressed="!annotator.tool.value"
                        :disabled="!selection"
                        @click="annotator.setTool(null)"
                    >
                        <IScan />
                    </UiButton>
                </template>
                <template #actions>
                    <UiButton
                        size="icon-sm"
                        variant="ghost"
                        aria-label="Cancel"
                        @click="handleCancel"
                    >
                        <IX />
                    </UiButton>
                    <UiButton
                        size="icon-sm"
                        aria-label="Capture"
                        :disabled="!selection"
                        @click="handleConfirm"
                    >
                        <ICheck />
                    </UiButton>
                </template>
            </AnnotationToolbar>
        </div>

        <div
            v-if="recordToolbar"
            ref="recordToolbarElement"
            class="absolute flex items-center gap-1 rounded-4xl border bg-popover p-1 text-popover-foreground shadow-md"
            :style="{ left: `${recordToolbar.x}px`, top: `${recordToolbar.y}px` }"
            @mousedown.stop
            @mouseup.stop
            @dblclick.stop
            @contextmenu.stop.prevent
        >
            <UiButton size="icon-sm" variant="ghost" aria-label="Cancel" @click="handleCancel">
                <IX />
            </UiButton>
            <UiButton size="sm" variant="destructive" @click="handleConfirm">
                <ICircle class="fill-current" />
                {{ payload?.purpose === 'gif' ? 'Record GIF' : 'Record' }}
            </UiButton>
        </div>
    </div>
</template>

<script setup lang="ts">
import type { LocalRect, OverlayFinish, OverlayPayload } from '../../shared/ipc';

type Point = { x: number; y: number };
type Handle = 'n' | 's' | 'e' | 'w' | 'ne' | 'nw' | 'se' | 'sw' | 'move';
type Mode = 'idle' | 'pending' | 'drawing' | 'selected' | 'moving' | 'resizing';
type ToolbarEdge = 'top' | 'bottom' | 'left' | 'right' | null;

const HANDLE_SIZE = 8;
const DRAG_THRESHOLD = 3;
const MAGNIFIER_PIXELS = 15;
const MAGNIFIER_ZOOM = 8;
const HOVER_ANIMATION_MS = 140;
const TOOLBAR_MARGIN = 16;
const TOOLBAR_EDGE = 96;
const TOOLBAR_EDGE_Y = 64;
const TOOLBAR_MAGNET = 72;

const CURSORS: Record<Handle, string> = {
    n: 'ns-resize',
    s: 'ns-resize',
    e: 'ew-resize',
    w: 'ew-resize',
    ne: 'nesw-resize',
    sw: 'nesw-resize',
    nw: 'nwse-resize',
    se: 'nwse-resize',
    move: 'move',
};

const baseCanvas = useTemplateRef('baseCanvas');
const drawCanvas = useTemplateRef('drawCanvas');
const annotationLayer = useTemplateRef('annotationLayer');
const captureToolbar = useTemplateRef('captureToolbar');
const recordToolbarElement = useTemplateRef('recordToolbarElement');

const annotator = useAnnotator(annotationLayer);
const captureToolbarSize = useElementSize(captureToolbar, undefined, { box: 'border-box' });
const recordToolbarSize = useElementSize(recordToolbarElement, undefined, {
    box: 'border-box',
});

const toolbarPlacement = useLocalStorage<{ edge: ToolbarEdge; cx: number; cy: number }>(
    'fileship-capture-toolbar-placement',
    { edge: 'top', cx: 0.5, cy: 0 },
);

const toolbarDrag = ref<{ id: number; offset: Point; pointer: Point } | null>(null);

const payload = shallowRef<OverlayPayload | null>(null);
const selection = ref<LocalRect | null>(null);
const mode = ref<Mode>('idle');
const cursor = ref('crosshair');

let pointer: Point | null = null;
let hover: LocalRect | null = null;
let hoverAnimation: { from: LocalRect; start: number } | null = null;
let dragStart: Point | null = null;
let dragOrigin: LocalRect | null = null;
let dragHandle: Handle | null = null;
let frame = 0;
let isFinishing = false;

const viewport = () => ({ x: 0, y: 0, width: window.innerWidth, height: window.innerHeight });

const ratio = () => (payload.value ? payload.value.width / window.innerWidth : 1);

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

const targetAt = (point: Point) => {
    const target = payload.value?.windows.find((item) => contains(item, point));
    if (!target) return viewport();

    const control = target.controls
        .filter((item) => contains(item, point))
        .sort((a, b) => a.width * a.height - b.width * b.height)[0];

    return intersect(control ?? target, viewport()) ?? viewport();
};

const handleAt = (point: Point): Handle | null => {
    const rect = selection.value;
    if (!rect) return null;

    const near = (value: number, edge: number) => Math.abs(value - edge) <= HANDLE_SIZE;
    const insideX = point.x > rect.x - HANDLE_SIZE && point.x < rect.x + rect.width + HANDLE_SIZE;
    const insideY = point.y > rect.y - HANDLE_SIZE && point.y < rect.y + rect.height + HANDLE_SIZE;
    if (!insideX || !insideY) return null;

    const vertical = near(point.y, rect.y) ? 'n' : near(point.y, rect.y + rect.height) ? 's' : '';
    const horizontal = near(point.x, rect.x) ? 'w' : near(point.x, rect.x + rect.width) ? 'e' : '';

    if (vertical || horizontal) return `${vertical}${horizontal}` as Handle;
    return contains(rect, point) ? 'move' : null;
};

const resizeRect = (origin: LocalRect, handle: Handle, dx: number, dy: number) => {
    let left = origin.x;
    let top = origin.y;
    let right = origin.x + origin.width;
    let bottom = origin.y + origin.height;

    if (handle.includes('w')) left += dx;
    if (handle.includes('e')) right += dx;
    if (handle.includes('n')) top += dy;
    if (handle.includes('s')) bottom += dy;

    return intersect(fromPoints({ x: left, y: top }, { x: right, y: bottom }), viewport());
};

const moveRect = (origin: LocalRect, dx: number, dy: number) => {
    const bounds = viewport();

    return {
        ...origin,
        x: clamp(origin.x + dx, 0, bounds.width - origin.width),
        y: clamp(origin.y + dy, 0, bounds.height - origin.height),
    };
};

const isRecording = computed(() => !!payload.value && payload.value.purpose !== 'capture');

const isAnnotating = computed(
    () => mode.value === 'selected' && !isRecording.value && !!annotator.tool.value,
);

const hasCaptureToolbar = computed(
    () =>
        !!payload.value &&
        payload.value.purpose === 'capture' &&
        !payload.value.quickCrop &&
        (payload.value.isActive || !!selection.value),
);

const edgeAt = (point: Point): ToolbarEdge => {
    const bounds = viewport();

    if (point.x < TOOLBAR_EDGE) return 'left';
    if (point.x > bounds.width - TOOLBAR_EDGE) return 'right';
    if (point.y < TOOLBAR_EDGE_Y) return 'top';
    if (point.y > bounds.height - TOOLBAR_EDGE_Y) return 'bottom';
    return null;
};

const magnet = (value: number, middle: number) =>
    Math.abs(value - middle) < TOOLBAR_MAGNET ? middle : value;

const layoutToolbar = (edge: ToolbarEdge, center: Point) => {
    const bounds = viewport();
    const width = captureToolbarSize.width.value;
    const height = captureToolbarSize.height.value;

    let x = center.x;
    let y = center.y;

    if (edge === 'left') x = TOOLBAR_MARGIN + width / 2;
    if (edge === 'right') x = bounds.width - TOOLBAR_MARGIN - width / 2;
    if (edge === 'top') y = TOOLBAR_MARGIN + height / 2;
    if (edge === 'bottom') y = bounds.height - TOOLBAR_MARGIN - height / 2;
    if (edge === 'left' || edge === 'right') y = magnet(y, bounds.height / 2);
    if (edge === 'top' || edge === 'bottom') x = magnet(x, bounds.width / 2);

    const left = clamp(
        x - width / 2,
        TOOLBAR_MARGIN,
        Math.max(TOOLBAR_MARGIN, bounds.width - width - TOOLBAR_MARGIN),
    );
    const top = clamp(
        y - height / 2,
        TOOLBAR_MARGIN,
        Math.max(TOOLBAR_MARGIN, bounds.height - height - TOOLBAR_MARGIN),
    );

    return { left, top, center: { x: left + width / 2, y: top + height / 2 } };
};

const currentEdge = computed(() =>
    toolbarDrag.value ? edgeAt(toolbarDrag.value.pointer) : toolbarPlacement.value.edge,
);

const currentLayout = computed(() => {
    const bounds = viewport();
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
              x: toolbarPlacement.value.cx * bounds.width,
              y: toolbarPlacement.value.cy * bounds.height,
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
    };
};

const handleToolbarDragMove = (event: PointerEvent) => {
    const drag = toolbarDrag.value;
    if (!drag || event.pointerId !== drag.id) return;

    toolbarDrag.value = { ...drag, pointer: { x: event.clientX, y: event.clientY } };
};

const handleToolbarDragEnd = (event: PointerEvent) => {
    const drag = toolbarDrag.value;
    if (!drag || event.pointerId !== drag.id) return;

    const bounds = viewport();
    const { center } = currentLayout.value;

    toolbarPlacement.value = {
        edge: currentEdge.value,
        cx: center.x / bounds.width,
        cy: center.y / bounds.height,
    };
    toolbarDrag.value = null;
};

const recordToolbar = computed(() => {
    const rect = selection.value;
    if (!rect || mode.value !== 'selected' || !isRecording.value) return null;

    const bounds = viewport();
    const width = recordToolbarSize.width.value || 82;
    const height = recordToolbarSize.height.value || 42;
    const x = clamp(rect.x + rect.width - width, 8, Math.max(8, bounds.width - width - 8));
    const below = rect.y + rect.height + 8;
    const above = rect.y - height - 8;

    if (below + height <= bounds.height) return { x, y: below };
    if (above >= 0) return { x, y: above };
    return { x, y: Math.max(8, rect.y + rect.height - height - 8) };
});

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
    const bounds = viewport();
    const size = MAGNIFIER_PIXELS * MAGNIFIER_ZOOM;
    const physical = toPhysical({ x: point.x, y: point.y, width: 0, height: 0 });
    const half = Math.floor(MAGNIFIER_PIXELS / 2);

    let x = point.x + 24;
    let y = point.y + 24;
    if (x + size > bounds.width) x = point.x - size - 24;
    if (y + size + 28 > bounds.height) y = point.y - size - 52;

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

    drawLabel(context, `${physical.x}, ${physical.y}`, x, y + size + 6);
};

const render = () => {
    frame = 0;

    const canvas = drawCanvas.value;
    if (!canvas || !payload.value) return;

    const context = canvas.getContext('2d')!;
    const bounds = viewport();
    const dpr = window.devicePixelRatio;
    const primary = themeColor('--primary');
    const region =
        selection.value ??
        (mode.value === 'idle' || mode.value === 'pending' ? animatedHover() : null);

    context.setTransform(dpr, 0, 0, dpr, 0, 0);
    context.clearRect(0, 0, bounds.width, bounds.height);

    context.fillStyle = 'rgba(0, 0, 0, 0.45)';
    context.fillRect(0, 0, bounds.width, bounds.height);

    if (region) {
        context.clearRect(region.x, region.y, region.width, region.height);

        context.strokeStyle = primary;
        context.lineWidth = selection.value ? 2 : 1.5;
        context.setLineDash(selection.value ? [] : [6, 4]);
        context.strokeRect(region.x, region.y, region.width, region.height);
        context.setLineDash([]);
    }

    if (selection.value) {
        const rect = selection.value;
        const physical = toPhysical(rect);
        const labelY = rect.y >= 28 ? rect.y - 26 : rect.y + 6;

        drawLabel(context, `${physical.width} × ${physical.height}`, rect.x, labelY);

        if (mode.value !== 'drawing') {
            const xs = [rect.x, rect.x + rect.width / 2, rect.x + rect.width];
            const ys = [rect.y, rect.y + rect.height / 2, rect.y + rect.height];

            context.fillStyle = primary;
            context.strokeStyle = '#ffffff';
            context.lineWidth = 1.5;

            for (const [i, x] of xs.entries()) {
                for (const [j, y] of ys.entries()) {
                    if (i === 1 && j === 1) continue;
                    context.beginPath();
                    context.rect(x - 4, y - 4, 8, 8);
                    context.fill();
                    context.stroke();
                }
            }
        }
    }

    const showsMagnifier =
        payload.value.showMagnifier &&
        pointer &&
        mode.value !== 'selected' &&
        mode.value !== 'moving';

    if (showsMagnifier) drawMagnifier(context, pointer!);
};

const scheduleRender = () => {
    if (!frame) frame = requestAnimationFrame(render);
};

const resizeDrawCanvas = () => {
    const canvas = drawCanvas.value;
    if (!canvas) return;

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
    dragStart = null;
    dragOrigin = null;
    dragHandle = null;
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
    if (isRecording.value) {
        return finish({
            type: 'area',
            displayId: payload.value!.displayId,
            region: { x: rect.x, y: rect.y, width: rect.width, height: rect.height },
        });
    }

    const canvas = annotator.renderRegion(rect);
    const { data } = canvas.getContext('2d')!.getImageData(0, 0, canvas.width, canvas.height);

    finish({
        type: 'region',
        displayId: payload.value!.displayId,
        region: { x: rect.x, y: rect.y, width: rect.width, height: rect.height },
        width: canvas.width,
        height: canvas.height,
        rgba: new Uint8Array(data.buffer),
    });
};

const pointFrom = (event: MouseEvent): Point => ({ x: event.clientX, y: event.clientY });

const handleMouseDown = (event: MouseEvent) => {
    if (event.button !== 0 || !payload.value || isAnnotating.value) return;

    const point = pointFrom(event);
    const handle = mode.value === 'selected' ? handleAt(point) : null;

    dragStart = point;

    if (handle) {
        dragOrigin = { ...selection.value! };
        dragHandle = handle;
        mode.value = handle === 'move' ? 'moving' : 'resizing';
        return;
    }

    selection.value = null;
    mode.value = 'pending';
    scheduleRender();
};

const handleMouseMove = (event: MouseEvent) => {
    if (!payload.value || isAnnotating.value) return;

    const point = pointFrom(event);
    pointer = point;

    if (mode.value === 'pending' && dragStart) {
        const distance = Math.hypot(point.x - dragStart.x, point.y - dragStart.y);
        if (distance >= DRAG_THRESHOLD) mode.value = 'drawing';
    }

    if (mode.value === 'drawing' && dragStart) {
        let end = point;

        if (event.shiftKey) {
            const size = Math.max(Math.abs(point.x - dragStart.x), Math.abs(point.y - dragStart.y));
            end = {
                x: dragStart.x + Math.sign(point.x - dragStart.x) * size,
                y: dragStart.y + Math.sign(point.y - dragStart.y) * size,
            };
        }

        selection.value = intersect(fromPoints(dragStart, end), viewport());
    } else if (mode.value === 'moving' && dragStart && dragOrigin) {
        selection.value = moveRect(dragOrigin, point.x - dragStart.x, point.y - dragStart.y);
    } else if (mode.value === 'resizing' && dragStart && dragOrigin && dragHandle) {
        selection.value =
            resizeRect(dragOrigin, dragHandle, point.x - dragStart.x, point.y - dragStart.y) ??
            selection.value;
    } else if (mode.value === 'idle') {
        setHover(targetAt(point));
    }

    if (mode.value === 'selected') {
        const handle = handleAt(point);
        cursor.value = handle ? CURSORS[handle] : 'crosshair';
    } else if (mode.value === 'idle' || mode.value === 'pending' || mode.value === 'drawing') {
        cursor.value = 'crosshair';
    }

    scheduleRender();
};

const handleMouseUp = (event: MouseEvent) => {
    if (event.button !== 0 || !payload.value || isAnnotating.value) return;

    if (mode.value === 'pending') {
        selection.value = targetAt(pointFrom(event));
    }

    dragStart = null;
    dragOrigin = null;
    dragHandle = null;

    if (!selection.value || selection.value.width < 1 || selection.value.height < 1) {
        selection.value = null;
        mode.value = 'idle';
        scheduleRender();
        return;
    }

    if (payload.value.quickCrop) return finishRegion(selection.value);

    mode.value = 'selected';
    scheduleRender();
};

const handleMouseLeave = () => {
    pointer = null;
    if (mode.value === 'idle') setHover(null);
    scheduleRender();
};

const handleDoubleClick = (event: MouseEvent) => {
    if (isAnnotating.value) return;

    if (selection.value && contains(selection.value, pointFrom(event))) {
        finishRegion(selection.value);
    }
};

const handleRightClick = () => {
    if (isAnnotating.value) {
        annotator.select(null);
        return;
    }

    if (mode.value === 'idle' && !selection.value) return finish({ type: 'cancel' });

    selection.value = null;
    mode.value = 'idle';
    if (pointer) setHover(targetAt(pointer));
    scheduleRender();
};

const handleCancel = () => finish({ type: 'cancel' });

const handleConfirm = () => {
    if (selection.value) finishRegion(selection.value);
};

const handleKeyDown = (event: KeyboardEvent) => {
    if (!payload.value || annotator.isEditingText.value) return;

    if (event.key === 'Escape') {
        if (annotator.selectedId.value) return annotator.select(null);
        return finish({ type: 'cancel' });
    }

    if (mode.value === 'selected' && !isRecording.value && annotator.handleKey(event)) {
        event.preventDefault();
        return;
    }

    if (event.key === ' ') return finish({ type: 'fullscreen' });

    if (event.key === 'Enter') {
        const rect = selection.value ?? hover;
        if (rect) finishRegion(rect);
        return;
    }

    if (/^[1-9]$/.test(event.key) && mode.value === 'idle') {
        return finish({ type: 'monitor', index: Number(event.key) - 1 });
    }

    const arrows: Record<string, Point> = {
        ArrowLeft: { x: -1, y: 0 },
        ArrowRight: { x: 1, y: 0 },
        ArrowUp: { x: 0, y: -1 },
        ArrowDown: { x: 0, y: 1 },
    };

    const arrow = arrows[event.key];
    if (!arrow || mode.value !== 'selected' || !selection.value || annotator.tool.value) return;

    event.preventDefault();

    const step = event.shiftKey ? 10 : 1;
    const dx = arrow.x * step;
    const dy = arrow.y * step;

    selection.value = event.ctrlKey
        ? (resizeRect(selection.value, 'se', dx, dy) ?? selection.value)
        : moveRect(selection.value, dx, dy);

    scheduleRender();
};

const nextFrame = () => new Promise((resolve) => requestAnimationFrame(resolve));

const handleOpen = async (value: OverlayPayload) => {
    reset();
    isFinishing = false;
    payload.value = value;

    const canvas = baseCanvas.value!;
    canvas.width = value.width;
    canvas.height = value.height;

    const bytes = value.pixels.byteOffset % 4 ? value.pixels.slice() : value.pixels;
    const source = new Uint32Array(bytes.buffer, bytes.byteOffset, bytes.length / 4);
    const rgba = new Uint8ClampedArray(source.length * 4);
    const target = new Uint32Array(rgba.buffer);

    for (let i = 0; i < source.length; i++) {
        const pixel = source[i]!;
        target[i] = (pixel & 0xff00ff00) | ((pixel & 0xff) << 16) | ((pixel >>> 16) & 0xff);
    }

    canvas.getContext('2d')!.putImageData(new ImageData(rgba, value.width, value.height), 0, 0);

    if (value.isActive) {
        pointer = value.cursor;
        hover = targetAt(value.cursor);
    }

    annotator.resize(window.innerWidth, window.innerHeight);
    annotator.setSource(canvas, ratio());
    resizeDrawCanvas();
    render();

    await nextFrame();
    await nextFrame();
    desktop('overlay:ready', value.displayId);
};

const handleClose = () => {
    payload.value = null;
    reset();

    for (const canvas of [baseCanvas.value, drawCanvas.value]) {
        canvas?.getContext('2d')?.clearRect(0, 0, canvas.width, canvas.height);
    }
};

useHead({
    style: [{ innerHTML: '#nuxt-devtools-container { display: none !important; }' }],
});

useEventListener('keydown', handleKeyDown);
useEventListener('resize', resizeDrawCanvas);

onMounted(() => {
    annotator.mount(window.innerWidth, window.innerHeight);
    annotator.tool.value = null;

    const offOpen = window.fileship.on('overlay:open', handleOpen);
    const offClose = window.fileship.on('overlay:close', handleClose);

    onBeforeUnmount(() => {
        offOpen();
        offClose();
    });
});

definePageMeta({
    layout: false,
});
</script>
