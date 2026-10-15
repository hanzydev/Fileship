import Konva from 'konva';

export const ANNOTATION_TOOLS = [
    'select',
    'rectangle',
    'ellipse',
    'arrow',
    'line',
    'freehand',
    'highlight',
    'text',
    'step',
    'blur',
    'pixelate',
    'eraser',
    'emoji',
] as const;

export type AnnotationTool = (typeof ANNOTATION_TOOLS)[number];

export const ANNOTATION_SHORTCUTS: Record<AnnotationTool, string> = {
    select: 'v',
    rectangle: 'r',
    ellipse: 'e',
    arrow: 'a',
    line: 'l',
    freehand: 'f',
    highlight: 'h',
    text: 't',
    step: 'n',
    blur: 'b',
    pixelate: 'p',
    eraser: 'x',
    emoji: 'j',
};

export const COLORED_TOOLS: AnnotationTool[] = [
    'rectangle',
    'ellipse',
    'arrow',
    'line',
    'freehand',
    'highlight',
    'text',
    'step',
];

export const SIZE_RANGES: Partial<Record<AnnotationTool, [number, number]>> = {
    rectangle: [1, 30],
    ellipse: [1, 30],
    arrow: [1, 30],
    line: [1, 30],
    freehand: [1, 30],
    text: [10, 120],
    step: [10, 60],
    blur: [2, 40],
    pixelate: [2, 40],
    emoji: [16, 256],
};

type Base = { id: string; x: number; y: number; scaleX: number; scaleY: number };

type BoxType = 'rectangle' | 'ellipse' | 'highlight' | 'blur' | 'pixelate' | 'eraser';

export type Annotation =
    | (Base & {
          type: BoxType;
          width: number;
          height: number;
          color: string;
          size: number;
          fill: boolean;
          shadow: boolean;
      })
    | (Base & {
          type: 'arrow' | 'line' | 'freehand';
          points: number[];
          color: string;
          size: number;
          shadow: boolean;
      })
    | (Base & {
          type: 'text';
          text: string;
          color: string;
          size: number;
          background: boolean;
          shadow: boolean;
      })
    | (Base & { type: 'step'; value: number; color: string; size: number; shadow: boolean })
    | (Base & { type: 'emoji'; text: string; size: number });

type ToolOptions = { color: string; size: number };

export type AnnotationOptions = {
    tools: Record<AnnotationTool, ToolOptions>;
    fill: boolean;
    background: boolean;
    shadow: boolean;
    emoji: string;
};

const RED = '#ef4444';

const DEFAULT_OPTIONS: AnnotationOptions = {
    tools: {
        select: { color: RED, size: 0 },
        rectangle: { color: RED, size: 4 },
        ellipse: { color: RED, size: 4 },
        arrow: { color: RED, size: 4 },
        line: { color: RED, size: 4 },
        freehand: { color: RED, size: 4 },
        highlight: { color: '#facc15', size: 0 },
        text: { color: RED, size: 28 },
        step: { color: RED, size: 18 },
        blur: { color: RED, size: 12 },
        pixelate: { color: RED, size: 12 },
        eraser: { color: RED, size: 0 },
        emoji: { color: RED, size: 64 },
    },
    fill: false,
    background: false,
    shadow: true,
    emoji: '👍',
};

const BOX_TYPES: string[] = ['rectangle', 'ellipse', 'highlight', 'blur', 'pixelate', 'eraser'];
const LINE_TYPES: string[] = ['arrow', 'line', 'freehand'];
const MIN_SIZE = 3;

const isBox = (shape: Annotation): shape is Extract<Annotation, { type: BoxType }> =>
    BOX_TYPES.includes(shape.type);

const isLine = (shape: Annotation): shape is Extract<Annotation, { points: number[] }> =>
    LINE_TYPES.includes(shape.type);

const contrastColor = (color: string) => {
    const value = Number.parseInt(color.replace('#', '').slice(0, 6), 16);
    const luminance =
        (0.299 * ((value >> 16) & 255) + 0.587 * ((value >> 8) & 255) + 0.114 * (value & 255)) /
        255;

    return luminance > 0.6 ? '#000000' : '#ffffff';
};

const shadowAttrs = (enabled: boolean) => ({
    shadowEnabled: enabled,
    shadowColor: '#000000',
    shadowBlur: 6,
    shadowOffset: { x: 1, y: 1 },
    shadowOpacity: 0.45,
});

const fontFamily = () =>
    import.meta.client ? getComputedStyle(document.body).fontFamily : 'sans-serif';

export const useAnnotator = (container: Ref<HTMLElement | null | undefined>) => {
    const tool = ref<AnnotationTool | null>('rectangle');
    const options = useLocalStorage<AnnotationOptions>('fileship-annotations', DEFAULT_OPTIONS, {
        mergeDefaults: (stored, defaults) => ({
            ...defaults,
            ...stored,
            tools: { ...defaults.tools, ...stored.tools },
        }),
    });

    const shapes = shallowRef<Annotation[]>([]);
    const selectedId = ref<string | null>(null);
    const isEditingText = ref(false);
    const past = shallowRef<Annotation[][]>([]);
    const future = shallowRef<Annotation[][]>([]);

    const nodes = new Map<string, Konva.Node>();
    let stage: Konva.Stage | null = null;
    let layer: Konva.Layer | null = null;
    let transformer: Konva.Transformer | null = null;
    let source: { canvas: HTMLCanvasElement; scale: number } | null = null;
    let draft: { shape: Annotation; start: { x: number; y: number } } | null = null;
    let editor: HTMLTextAreaElement | null = null;

    const canUndo = computed(() => past.value.length > 0);
    const canRedo = computed(() => future.value.length > 0);
    const hasShapes = computed(() => shapes.value.length > 0);

    const findShape = (id: string | null) => shapes.value.find((shape) => shape.id === id) ?? null;

    const activeTool = computed<AnnotationTool>(() => {
        const selected = findShape(selectedId.value);
        if (tool.value === 'select' && selected) return selected.type;
        return tool.value ?? 'select';
    });

    const current = computed(() => options.value.tools[activeTool.value]);

    const commit = (next: Annotation[]) => {
        past.value = [...past.value.slice(-99), shapes.value];
        future.value = [];
        shapes.value = next;
    };

    const replaceShape = (shape: Annotation) =>
        commit(shapes.value.map((item) => (item.id === shape.id ? shape : item)));

    const sourceRect = (shape: { x: number; y: number; width: number; height: number }) => {
        const scale = source?.scale ?? 1;

        return {
            x: Math.round(shape.x * scale),
            y: Math.round(shape.y * scale),
            width: Math.max(1, Math.round(shape.width * scale)),
            height: Math.max(1, Math.round(shape.height * scale)),
        };
    };

    const sampleColor = (point: { x: number; y: number }) => {
        if (!source) return '#ffffff';

        const context = source.canvas.getContext('2d', { willReadFrequently: true })!;
        const [r, g, b] = context.getImageData(
            Math.round(point.x * source.scale),
            Math.round(point.y * source.scale),
            1,
            1,
        ).data;

        return `#${[r, g, b].map((value) => value!.toString(16).padStart(2, '0')).join('')}`;
    };

    const refreshEffect = (node: Konva.Image, shape: Extract<Annotation, { type: BoxType }>) => {
        if (!source || shape.width < 1 || shape.height < 1) return;

        node.clearCache();
        node.setAttrs({
            image: source.canvas,
            width: shape.width,
            height: shape.height,
            crop: sourceRect(shape),
        });

        if (shape.type === 'blur') {
            node.filters([Konva.Filters.Blur]);
            node.blurRadius(shape.size * source.scale);
        } else {
            node.filters([Konva.Filters.Pixelate]);
            node.pixelSize(Math.max(2, Math.round(shape.size * source.scale)));
        }

        node.cache({ pixelRatio: source.scale });
    };

    const createNode = (shape: Annotation): Konva.Node => {
        const base = {
            id: shape.id,
            x: shape.x,
            y: shape.y,
            scaleX: shape.scaleX,
            scaleY: shape.scaleY,
        };

        switch (shape.type) {
            case 'rectangle':
                return new Konva.Rect({
                    ...base,
                    width: shape.width,
                    height: shape.height,
                    stroke: shape.color,
                    strokeWidth: shape.size,
                    fill: shape.fill ? shape.color : undefined,
                    cornerRadius: Math.min(shape.size, 6),
                    strokeScaleEnabled: false,
                    ...shadowAttrs(shape.shadow),
                });
            case 'ellipse':
                return new Konva.Ellipse({
                    ...base,
                    x: shape.x + shape.width / 2,
                    y: shape.y + shape.height / 2,
                    radiusX: shape.width / 2,
                    radiusY: shape.height / 2,
                    stroke: shape.color,
                    strokeWidth: shape.size,
                    fill: shape.fill ? shape.color : undefined,
                    strokeScaleEnabled: false,
                    ...shadowAttrs(shape.shadow),
                });
            case 'highlight':
                return new Konva.Rect({
                    ...base,
                    width: shape.width,
                    height: shape.height,
                    fill: shape.color,
                    opacity: 0.4,
                });
            case 'eraser':
                return new Konva.Rect({
                    ...base,
                    width: shape.width,
                    height: shape.height,
                    fill: shape.color,
                });
            case 'blur':
            case 'pixelate': {
                const node = new Konva.Image({ ...base, image: undefined });
                refreshEffect(node, shape);
                return node;
            }
            case 'arrow':
                return new Konva.Arrow({
                    ...base,
                    points: shape.points,
                    stroke: shape.color,
                    fill: shape.color,
                    strokeWidth: shape.size,
                    pointerLength: shape.size * 3 + 6,
                    pointerWidth: shape.size * 3 + 6,
                    lineCap: 'round',
                    lineJoin: 'round',
                    hitStrokeWidth: Math.max(12, shape.size),
                    ...shadowAttrs(shape.shadow),
                });
            case 'line':
            case 'freehand':
                return new Konva.Line({
                    ...base,
                    points: shape.points,
                    stroke: shape.color,
                    strokeWidth: shape.size,
                    lineCap: 'round',
                    lineJoin: 'round',
                    tension: shape.type === 'freehand' ? 0.4 : 0,
                    hitStrokeWidth: Math.max(12, shape.size),
                    ...shadowAttrs(shape.shadow),
                });
            case 'text': {
                const label = new Konva.Label({ ...base, ...shadowAttrs(false) });
                const padding = shape.background ? Math.round(shape.size * 0.35) : 0;

                label.add(
                    new Konva.Tag({
                        fill: shape.background ? shape.color : undefined,
                        cornerRadius: padding,
                        ...shadowAttrs(shape.shadow && shape.background),
                    }),
                );
                label.add(
                    new Konva.Text({
                        text: shape.text || ' ',
                        fontSize: shape.size,
                        fontFamily: fontFamily(),
                        fontStyle: 'bold',
                        lineHeight: 1.2,
                        padding,
                        fill: shape.background ? contrastColor(shape.color) : shape.color,
                        ...shadowAttrs(shape.shadow && !shape.background),
                    }),
                );

                return label;
            }
            case 'step': {
                const group = new Konva.Group(base);

                group.add(
                    new Konva.Circle({
                        radius: shape.size,
                        fill: shape.color,
                        stroke: '#ffffff',
                        strokeWidth: Math.max(2, shape.size / 8),
                        ...shadowAttrs(shape.shadow),
                    }),
                );
                group.add(
                    new Konva.Text({
                        text: String(shape.value),
                        width: shape.size * 2,
                        height: shape.size * 2,
                        offsetX: shape.size,
                        offsetY: shape.size,
                        align: 'center',
                        verticalAlign: 'middle',
                        fontSize: shape.size,
                        fontStyle: 'bold',
                        fontFamily: fontFamily(),
                        fill: contrastColor(shape.color),
                    }),
                );

                return group;
            }
            case 'emoji':
                return new Konva.Text({
                    ...base,
                    text: shape.text,
                    fontSize: shape.size,
                    fontFamily:
                        '"Segoe UI Emoji", "Apple Color Emoji", "Noto Color Emoji", sans-serif',
                    ...shadowAttrs(false),
                });
        }
    };

    const isSelectable = () => tool.value === 'select' || tool.value === null;

    const attachNode = (shape: Annotation, node: Konva.Node) => {
        node.draggable(isSelectable() || selectedId.value === shape.id);

        node.on('pointerdown', (event) => {
            if (!isSelectable() && selectedId.value !== shape.id) return;
            event.cancelBubble = true;
            select(shape.id);
        });

        node.on('dragmove', () => {
            const item = findShape(shape.id);
            if (!item || (item.type !== 'blur' && item.type !== 'pixelate')) return;

            const offset = { x: node.x(), y: node.y() };
            refreshEffect(node as Konva.Image, { ...item, ...offset });
        });

        node.on('dragend', () => {
            const item = findShape(shape.id);
            if (!item) return;

            const position =
                item.type === 'ellipse'
                    ? { x: node.x() - item.width / 2, y: node.y() - item.height / 2 }
                    : { x: node.x(), y: node.y() };

            replaceShape({ ...item, ...position });
        });

        node.on('transformend', () => handleTransformEnd(shape.id, node));

        node.on('dblclick dbltap', () => {
            const item = findShape(shape.id);
            if (item?.type === 'text') editText(item);
        });

        layer!.add(node as Konva.Shape);
        nodes.set(shape.id, node);
    };

    const handleTransformEnd = (id: string, node: Konva.Node) => {
        const item = findShape(id);
        if (!item) return;

        const scaleX = node.scaleX();
        const scaleY = node.scaleY();

        if (isBox(item)) {
            const width = Math.max(MIN_SIZE, item.width * scaleX);
            const height = Math.max(MIN_SIZE, item.height * scaleY);
            const position =
                item.type === 'ellipse'
                    ? { x: node.x() - width / 2, y: node.y() - height / 2 }
                    : { x: node.x(), y: node.y() };

            replaceShape({ ...item, ...position, width, height, scaleX: 1, scaleY: 1 });
        } else if (isLine(item)) {
            const points = item.points.map((value, index) => value * (index % 2 ? scaleY : scaleX));
            replaceShape({ ...item, x: node.x(), y: node.y(), points, scaleX: 1, scaleY: 1 });
        } else {
            replaceShape({ ...item, x: node.x(), y: node.y(), scaleX, scaleY });
        }
    };

    const updateTransformer = () => {
        if (!transformer) return;

        const shape = findShape(selectedId.value);
        const node = selectedId.value ? nodes.get(selectedId.value) : null;

        if (!shape || !node || isEditingText.value) {
            transformer.nodes([]);
            return;
        }

        const keepRatio = shape.type === 'text' || shape.type === 'emoji' || shape.type === 'step';

        transformer.setAttrs({
            keepRatio,
            rotateEnabled: false,
            enabledAnchors: keepRatio
                ? ['top-left', 'top-right', 'bottom-left', 'bottom-right']
                : undefined,
        });
        transformer.nodes([node]);
        transformer.moveToTop();
    };

    const render = () => {
        if (!layer) return;

        for (const node of nodes.values()) node.destroy();
        nodes.clear();

        for (const shape of shapes.value) attachNode(shape, createNode(shape));

        if (selectedId.value && !findShape(selectedId.value)) selectedId.value = null;
        for (const [id, node] of nodes) node.draggable(isSelectable() || selectedId.value === id);

        updateTransformer();
        layer.batchDraw();
    };

    const select = (id: string | null) => {
        selectedId.value = id;
        for (const [nodeId, node] of nodes) node.draggable(isSelectable() || nodeId === id);
        updateTransformer();
        layer?.batchDraw();
    };

    const pointer = () => layer!.getRelativePointerPosition() ?? { x: 0, y: 0 };

    const isOverShape = (point: { x: number; y: number }) => !!stage?.getIntersection(point);

    const newId = () => Math.random().toString(36).slice(2, 10);

    const startShape = (point: { x: number; y: number }): Annotation | null => {
        const { color, size } = options.value.tools[tool.value ?? 'select'];
        const { fill, shadow, background } = options.value;
        const base = { id: newId(), x: point.x, y: point.y, scaleX: 1, scaleY: 1 };

        switch (tool.value) {
            case 'rectangle':
            case 'ellipse':
            case 'highlight':
            case 'blur':
            case 'pixelate':
                return {
                    ...base,
                    type: tool.value,
                    width: 0,
                    height: 0,
                    color,
                    size,
                    fill,
                    shadow,
                };
            case 'eraser':
                return {
                    ...base,
                    type: 'eraser',
                    width: 0,
                    height: 0,
                    color: sampleColor(point),
                    size,
                    fill: true,
                    shadow: false,
                };
            case 'arrow':
            case 'line':
                return { ...base, type: tool.value, points: [0, 0, 0, 0], color, size, shadow };
            case 'freehand':
                return { ...base, type: 'freehand', points: [0, 0], color, size, shadow };
            case 'text':
                return { ...base, type: 'text', text: '', color, size, background, shadow };
            case 'step': {
                const steps = shapes.value.filter((shape) => shape.type === 'step');
                const value = Math.max(0, ...steps.map((shape) => shape.value)) + 1;
                return { ...base, type: 'step', value, color, size, shadow };
            }
            case 'emoji':
                return {
                    ...base,
                    x: point.x - size / 2,
                    y: point.y - size / 2,
                    type: 'emoji',
                    text: options.value.emoji,
                    size,
                };
            default:
                return null;
        }
    };

    const updateDraft = (point: { x: number; y: number }, isSquare: boolean) => {
        if (!draft) return;

        const { shape, start } = draft;
        let dx = point.x - start.x;
        let dy = point.y - start.y;

        if (isSquare) {
            const length = Math.max(Math.abs(dx), Math.abs(dy));
            dx = Math.sign(dx || 1) * length;
            dy = Math.sign(dy || 1) * length;
        }

        if (isBox(shape)) {
            draft.shape = {
                ...shape,
                x: Math.min(start.x, start.x + dx),
                y: Math.min(start.y, start.y + dy),
                width: Math.abs(dx),
                height: Math.abs(dy),
            };
        } else if (shape.type === 'freehand') {
            draft.shape = {
                ...shape,
                points: [...shape.points, point.x - start.x, point.y - start.y],
            };
        } else if (isLine(shape)) {
            draft.shape = { ...shape, points: [0, 0, dx, dy] };
        }

        nodes.get(shape.id)?.destroy();
        nodes.delete(shape.id);
        attachNode(draft.shape, createNode(draft.shape));
        layer?.batchDraw();
    };

    const finishDraft = () => {
        if (!draft) return;

        cancelAnimationFrame(moveFrame);

        const { shape } = draft;
        draft = null;

        const isTooSmall = isBox(shape)
            ? shape.width < MIN_SIZE || shape.height < MIN_SIZE
            : isLine(shape)
              ? Math.max(...shape.points.map(Math.abs)) < MIN_SIZE
              : false;

        if (isTooSmall) {
            nodes.get(shape.id)?.destroy();
            nodes.delete(shape.id);
            layer?.batchDraw();
            return;
        }

        commit([...shapes.value, shape]);
        render();
        select(shape.id);
    };

    const handlePointerDown = (event: Konva.KonvaEventObject<PointerEvent>) => {
        if (event.evt.button !== 0 || isEditingText.value) return;
        if (event.target.findAncestor('Transformer')) return;

        if (tool.value === 'select' || !tool.value) {
            if (event.target === stage) select(null);
            return;
        }

        select(null);

        const point = pointer();
        const shape = startShape(point);
        if (!shape) return;

        if (shape.type === 'text') {
            editText(shape, true);
            return;
        }

        if (shape.type === 'step' || shape.type === 'emoji') {
            commit([...shapes.value, shape]);
            render();
            select(shape.id);
            return;
        }

        draft = { shape, start: point };
        attachNode(shape, createNode(shape));
    };

    let moveFrame = 0;

    const handlePointerMove = (event: Konva.KonvaEventObject<PointerEvent>) => {
        if (!draft) return;

        const point = pointer();
        const isSquare = event.evt.shiftKey;

        if (draft.shape.type === 'freehand') return updateDraft(point, isSquare);

        cancelAnimationFrame(moveFrame);
        moveFrame = requestAnimationFrame(() => updateDraft(point, isSquare));
    };

    const editText = (shape: Extract<Annotation, { type: 'text' }>, isNew = false) => {
        if (!stage || !container.value) return;

        finishTextEdit();
        isEditingText.value = true;

        if (isNew) {
            attachNode(shape, createNode(shape));
        }

        const node = nodes.get(shape.id) as Konva.Label | undefined;
        if (!node) return;

        node.hide();
        updateTransformer();

        const scale = stage.scaleX() * shape.scaleX;
        const padding = shape.background ? Math.round(shape.size * 0.35) : 0;
        const position = node.getAbsolutePosition();
        const textarea = document.createElement('textarea');

        textarea.value = shape.text;
        textarea.spellcheck = false;
        textarea.rows = 1;
        textarea.cols = 1;
        Object.assign(textarea.style, {
            position: 'absolute',
            left: `${position.x}px`,
            top: `${position.y}px`,
            minWidth: `${shape.size * scale * 2}px`,
            margin: '0',
            padding: `${padding * scale}px`,
            border: '1px dashed currentColor',
            borderRadius: `${padding * scale}px`,
            outline: 'none',
            resize: 'none',
            overflow: 'hidden',
            whiteSpace: 'pre',
            font: `bold ${shape.size * scale}px/1.2 ${fontFamily()}`,
            color: shape.background ? contrastColor(shape.color) : shape.color,
            background: shape.background ? shape.color : 'transparent',
            zIndex: '10',
        });

        const fit = () => {
            textarea.style.height = 'auto';
            textarea.style.width = 'auto';
            textarea.style.height = `${textarea.scrollHeight}px`;
            textarea.style.width = `${textarea.scrollWidth + 4}px`;
        };

        textarea.addEventListener('input', fit);
        textarea.addEventListener('keydown', (event) => {
            event.stopPropagation();

            if (event.key === 'Escape' || (event.key === 'Enter' && !event.shiftKey)) {
                event.preventDefault();
                finishTextEdit();
            }
        });
        textarea.addEventListener('blur', () => finishTextEdit());
        textarea.addEventListener('pointerdown', (event) => event.stopPropagation());

        container.value.appendChild(textarea);
        editor = textarea;
        fit();
        requestAnimationFrame(() => textarea.focus());

        textarea.dataset.id = shape.id;
        textarea.dataset.isNew = String(isNew);
        textarea.dataset.shape = JSON.stringify(shape);
    };

    const finishTextEdit = () => {
        const textarea = editor;
        if (!textarea) return;

        editor = null;
        isEditingText.value = false;

        const shape = JSON.parse(textarea.dataset.shape!) as Extract<Annotation, { type: 'text' }>;
        const isNew = textarea.dataset.isNew === 'true';
        const text = textarea.value.replace(/\s+$/, '');

        textarea.remove();

        if (!text) {
            if (!isNew) commit(shapes.value.filter((item) => item.id !== shape.id));
            render();
            return;
        }

        const next = { ...shape, text };

        if (isNew) {
            commit([...shapes.value, next]);
        } else {
            replaceShape(next);
        }

        render();
        select(next.id);
    };

    const mount = (width: number, height: number, scale = 1) => {
        if (!container.value) return;

        stage?.destroy();
        nodes.clear();

        stage = new Konva.Stage({
            container: container.value as HTMLDivElement,
            width: width * scale,
            height: height * scale,
            scale: { x: scale, y: scale },
        });

        layer = new Konva.Layer();
        transformer = new Konva.Transformer({
            anchorSize: 8,
            anchorCornerRadius: 2,
            borderStroke: getComputedStyle(document.documentElement).getPropertyValue('--primary'),
            anchorStroke: getComputedStyle(document.documentElement).getPropertyValue('--primary'),
            ignoreStroke: true,
            flipEnabled: false,
        });

        layer.add(transformer);
        stage.add(layer);

        stage.on('pointerdown', handlePointerDown);
        stage.on('pointermove', handlePointerMove);
        stage.on('pointerup pointercancel', finishDraft);

        render();
    };

    const resize = (width: number, height: number, scale = 1) => {
        if (!stage) return mount(width, height, scale);

        finishTextEdit();
        stage.size({ width: width * scale, height: height * scale });
        stage.scale({ x: scale, y: scale });
        stage.batchDraw();
    };

    const setSource = (canvas: HTMLCanvasElement, scale = 1) => {
        source = { canvas, scale };
        render();
    };

    const unmount = () => {
        finishTextEdit();
        stage?.destroy();
        stage = null;
        layer = null;
        transformer = null;
        source = null;
        nodes.clear();
    };

    const reset = () => {
        finishTextEdit();
        draft = null;
        shapes.value = [];
        past.value = [];
        future.value = [];
        selectedId.value = null;
        render();
    };

    const undo = () => {
        const previous = past.value.at(-1);
        if (!previous) return;

        finishTextEdit();
        future.value = [shapes.value, ...future.value];
        past.value = past.value.slice(0, -1);
        shapes.value = previous;
        render();
    };

    const redo = () => {
        const next = future.value[0];
        if (!next) return;

        finishTextEdit();
        past.value = [...past.value, shapes.value];
        future.value = future.value.slice(1);
        shapes.value = next;
        render();
    };

    const removeSelected = () => {
        if (!selectedId.value) return false;

        commit(shapes.value.filter((shape) => shape.id !== selectedId.value));
        selectedId.value = null;
        render();
        return true;
    };

    const clear = () => {
        if (!shapes.value.length) return;
        commit([]);
        render();
    };

    const renderRegion = (rect: { x: number; y: number; width: number; height: number }) => {
        finishTextEdit();

        const scale = source?.scale ?? 1;
        const target = sourceRect(rect);
        const canvas = document.createElement('canvas');
        canvas.width = target.width;
        canvas.height = target.height;

        const context = canvas.getContext('2d')!;
        if (source) {
            context.drawImage(
                source.canvas,
                target.x,
                target.y,
                target.width,
                target.height,
                0,
                0,
                target.width,
                target.height,
            );
        }

        if (stage && shapes.value.length) {
            const stageScale = stage.scaleX();
            const size = stage.size();

            transformer?.nodes([]);
            stage.scale({ x: 1, y: 1 });
            stage.size({ width: size.width / stageScale, height: size.height / stageScale });

            const overlay = stage.toCanvas({
                x: rect.x,
                y: rect.y,
                width: rect.width,
                height: rect.height,
                pixelRatio: scale,
            });

            stage.scale({ x: stageScale, y: stageScale });
            stage.size(size);
            updateTransformer();

            context.drawImage(overlay, 0, 0, target.width, target.height);
        }

        return canvas;
    };

    const setTool = (value: AnnotationTool | null) => {
        tool.value = value;
    };

    const setToolOption = (key: keyof ToolOptions, value: string | number) => {
        options.value.tools[activeTool.value] = { ...current.value, [key]: value };
    };

    const setOption = <K extends 'fill' | 'background' | 'shadow' | 'emoji'>(
        key: K,
        value: AnnotationOptions[K],
    ) => {
        options.value[key] = value;
    };

    const handleKey = (event: KeyboardEvent) => {
        if (isEditingText.value) return false;

        const key = event.key.toLowerCase();

        if ((event.ctrlKey || event.metaKey) && key === 'z') {
            if (event.shiftKey) {
                redo();
            } else {
                undo();
            }
            return true;
        }

        if ((event.ctrlKey || event.metaKey) && key === 'y') {
            redo();
            return true;
        }

        if (event.key === 'Delete' || event.key === 'Backspace') return removeSelected();

        if (!event.ctrlKey && !event.altKey && !event.metaKey) {
            const match = ANNOTATION_TOOLS.find((item) => ANNOTATION_SHORTCUTS[item] === key);

            if (match) {
                tool.value = match;
                return true;
            }
        }

        return false;
    };

    watch(tool, () => {
        finishTextEdit();
        if (tool.value !== 'select') select(null);
        for (const [id, node] of nodes) node.draggable(isSelectable() || id === selectedId.value);

        if (container.value) {
            container.value.style.cursor =
                tool.value === null
                    ? ''
                    : tool.value === 'select'
                      ? 'default'
                      : tool.value === 'text'
                        ? 'text'
                        : 'crosshair';
        }
    });

    watch(
        () => [
            options.value.fill,
            options.value.background,
            options.value.shadow,
            current.value.color,
            current.value.size,
        ],
        () => {
            const shape = findShape(selectedId.value);
            if (!shape || shape.type !== activeTool.value) return;

            const next = {
                ...shape,
                ...('color' in shape && shape.type !== 'eraser'
                    ? { color: current.value.color }
                    : {}),
                size: current.value.size,
                ...('fill' in shape && shape.type !== 'eraser' ? { fill: options.value.fill } : {}),
                ...('background' in shape ? { background: options.value.background } : {}),
                ...('shadow' in shape ? { shadow: options.value.shadow } : {}),
            } as Annotation;

            if (JSON.stringify(next) === JSON.stringify(shape)) return;

            replaceShape(next);
            render();
        },
    );

    onBeforeUnmount(() => {
        finishTextEdit();
        stage?.destroy();
    });

    return {
        tool,
        activeTool,
        options,
        current,
        shapes,
        selectedId,
        isEditingText,
        canUndo,
        canRedo,
        hasShapes,
        mount,
        unmount,
        resize,
        setSource,
        reset,
        undo,
        redo,
        removeSelected,
        clear,
        renderRegion,
        handleKey,
        isOverShape,
        select,
        setTool,
        setToolOption,
        setOption,
    };
};

export type Annotator = ReturnType<typeof useAnnotator>;
