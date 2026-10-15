<template>
    <div :class="cn('flex items-center gap-1', isVertical ? 'flex-col' : 'flex-wrap')">
        <UiTooltipProvider :delay-duration="300">
            <slot name="start" />

            <UiTooltip v-for="tool in tools" :key="tool.value">
                <UiTooltipTrigger as-child>
                    <UiButton
                        :variant="annotator.tool.value === tool.value ? 'secondary' : 'ghost'"
                        size="icon-sm"
                        :aria-label="tool.label"
                        :aria-pressed="annotator.tool.value === tool.value"
                        :disabled="disabled"
                        @click="annotator.setTool(tool.value)"
                    >
                        <component :is="tool.icon" />
                    </UiButton>
                </UiTooltipTrigger>
                <UiTooltipContent :side="side">
                    {{ tool.label }}
                    <UiKbd class="ml-1">
                        {{ ANNOTATION_SHORTCUTS[tool.value].toUpperCase() }}
                    </UiKbd>
                </UiTooltipContent>
            </UiTooltip>

            <UiSeparator :orientation="separatorOrientation" :class="separatorClass" />

            <UiPopover v-if="isColored">
                <UiPopoverTrigger as-child>
                    <UiButton
                        variant="ghost"
                        size="icon-sm"
                        aria-label="Color"
                        :disabled="disabled"
                    >
                        <span
                            class="size-4 rounded-full border border-border"
                            :style="{ background: annotator.current.value.color }"
                        />
                    </UiButton>
                </UiPopoverTrigger>
                <UiPopoverContent class="w-auto" align="center" :side="side">
                    <div class="grid grid-cols-5 gap-2">
                        <button
                            v-for="color in COLORS"
                            :key="color"
                            type="button"
                            :aria-label="color"
                            :class="
                                cn(
                                    'size-7 rounded-full border border-border transition-transform hover:scale-110',
                                    annotator.current.value.color === color &&
                                        'ring-2 ring-ring ring-offset-2 ring-offset-popover',
                                )
                            "
                            :style="{ background: color }"
                            @click="handleColor(color)"
                        />
                        <label
                            class="relative flex size-7 items-center justify-center overflow-hidden rounded-full border border-border bg-muted"
                            aria-label="Custom color"
                        >
                            <IPipette class="size-3.5" />
                            <input
                                type="color"
                                class="absolute inset-0 opacity-0"
                                :value="annotator.current.value.color"
                                @input="handleColor(($event.target as HTMLInputElement).value)"
                            />
                        </label>
                    </div>
                </UiPopoverContent>
            </UiPopover>

            <UiPopover v-if="annotator.activeTool.value === 'emoji'">
                <UiPopoverTrigger as-child>
                    <UiButton variant="ghost" size="icon-sm" aria-label="Emoji">
                        <span class="text-base leading-none">
                            {{ annotator.options.value.emoji }}
                        </span>
                    </UiButton>
                </UiPopoverTrigger>
                <UiPopoverContent class="w-72" align="center" :side="side">
                    <div class="grid grid-cols-8 gap-1">
                        <UiButton
                            v-for="emoji in EMOJIS"
                            :key="emoji"
                            variant="ghost"
                            size="icon-sm"
                            class="text-lg"
                            @click="annotator.setOption('emoji', emoji)"
                        >
                            {{ emoji }}
                        </UiButton>
                    </div>
                </UiPopoverContent>
            </UiPopover>

            <UiPopover v-if="range || hasToggles">
                <UiPopoverTrigger as-child>
                    <UiButton
                        variant="ghost"
                        size="icon-sm"
                        aria-label="Options"
                        :disabled="disabled"
                    >
                        <ISlidersHorizontal />
                    </UiButton>
                </UiPopoverTrigger>
                <UiPopoverContent class="w-64" align="center" :side="side">
                    <UiFieldGroup>
                        <UiField v-if="range">
                            <UiFieldLabel>
                                {{ sizeLabel }} · {{ annotator.current.value.size }}
                            </UiFieldLabel>
                            <UiSlider
                                :model-value="[annotator.current.value.size]"
                                :min="range[0]"
                                :max="range[1]"
                                :step="1"
                                @update:model-value="handleSize($event?.[0])"
                            />
                        </UiField>
                        <UiField
                            v-for="toggle in toggles"
                            :key="toggle.key"
                            orientation="horizontal"
                            class="has-[>[data-slot=field-content]]:items-center"
                        >
                            <UiFieldContent>
                                <UiFieldLabel :for="`annotation-${toggle.key}`">
                                    {{ toggle.label }}
                                </UiFieldLabel>
                            </UiFieldContent>
                            <UiSwitch
                                :id="`annotation-${toggle.key}`"
                                :model-value="annotator.options.value[toggle.key]"
                                @update:model-value="annotator.setOption(toggle.key, $event)"
                            />
                        </UiField>
                    </UiFieldGroup>
                </UiPopoverContent>
            </UiPopover>

            <UiTooltip>
                <UiTooltipTrigger as-child>
                    <UiButton
                        variant="ghost"
                        size="icon-sm"
                        aria-label="Undo"
                        :disabled="!annotator.canUndo.value"
                        @click="annotator.undo"
                    >
                        <IUndo2 />
                    </UiButton>
                </UiTooltipTrigger>
                <UiTooltipContent :side="side">
                    Undo
                    <UiKbd class="ml-1">Ctrl Z</UiKbd>
                </UiTooltipContent>
            </UiTooltip>
            <UiTooltip>
                <UiTooltipTrigger as-child>
                    <UiButton
                        variant="ghost"
                        size="icon-sm"
                        aria-label="Redo"
                        :disabled="!annotator.canRedo.value"
                        @click="annotator.redo"
                    >
                        <IRedo2 />
                    </UiButton>
                </UiTooltipTrigger>
                <UiTooltipContent :side="side">
                    Redo
                    <UiKbd class="ml-1">Ctrl Y</UiKbd>
                </UiTooltipContent>
            </UiTooltip>
            <UiTooltip>
                <UiTooltipTrigger as-child>
                    <UiButton
                        variant="ghost"
                        size="icon-sm"
                        aria-label="Delete"
                        :disabled="!annotator.selectedId.value"
                        @click="annotator.removeSelected"
                    >
                        <ITrash2 />
                    </UiButton>
                </UiTooltipTrigger>
                <UiTooltipContent :side="side">
                    Delete
                    <UiKbd class="ml-1">Del</UiKbd>
                </UiTooltipContent>
            </UiTooltip>

            <template v-if="$slots.actions">
                <UiSeparator :orientation="separatorOrientation" :class="separatorClass" />
                <slot name="actions" />
            </template>
        </UiTooltipProvider>
    </div>
</template>

<script setup lang="ts">
import type { Component } from 'vue';

import {
    ICircle,
    IDroplets,
    IEraser,
    IGrid3x3,
    IHighlighter,
    IListOrdered,
    IMinus,
    IMousePointer2,
    IMoveUpRight,
    IPencil,
    ISmile,
    ISquare,
    IType,
} from '#components';

const {
    annotator,
    side = 'bottom',
    disabled = false,
} = defineProps<{
    annotator: Annotator;
    side?: 'top' | 'bottom' | 'left' | 'right';
    disabled?: boolean;
}>();

const COLORS = [
    '#ef4444',
    '#f97316',
    '#facc15',
    '#22c55e',
    '#06b6d4',
    '#3b82f6',
    '#a855f7',
    '#ec4899',
    '#ffffff',
    '#000000',
];

const EMOJIS = [
    '👍',
    '👎',
    '👀',
    '😀',
    '😂',
    '🥲',
    '😍',
    '🤔',
    '😮',
    '😡',
    '😭',
    '🙏',
    '👏',
    '🔥',
    '💯',
    '✅',
    '❌',
    '⚠️',
    '❗',
    '❓',
    '⭐',
    '❤️',
    '💡',
    '🎉',
    '🚀',
    '🐛',
    '📌',
    '👉',
    '👈',
    '👆',
    '👇',
    '🤝',
];

const TOOL_META: Record<AnnotationTool, { label: string; icon: Component }> = {
    select: { label: 'Select', icon: IMousePointer2 },
    rectangle: { label: 'Rectangle', icon: ISquare },
    ellipse: { label: 'Ellipse', icon: ICircle },
    arrow: { label: 'Arrow', icon: IMoveUpRight },
    line: { label: 'Line', icon: IMinus },
    freehand: { label: 'Pen', icon: IPencil },
    highlight: { label: 'Highlighter', icon: IHighlighter },
    text: { label: 'Text', icon: IType },
    step: { label: 'Step number', icon: IListOrdered },
    blur: { label: 'Blur', icon: IDroplets },
    pixelate: { label: 'Pixelate', icon: IGrid3x3 },
    eraser: { label: 'Smart eraser', icon: IEraser },
    emoji: { label: 'Emoji', icon: ISmile },
};

const tools = ANNOTATION_TOOLS.map((value) => ({ value, ...TOOL_META[value] }));

const isColored = computed(() => COLORED_TOOLS.includes(annotator.activeTool.value));

const range = computed(() => SIZE_RANGES[annotator.activeTool.value]);

const sizeLabel = computed(() => {
    const tool = annotator.activeTool.value;
    if (tool === 'text' || tool === 'emoji') return 'Size';
    if (tool === 'step') return 'Radius';
    if (tool === 'blur' || tool === 'pixelate') return 'Strength';
    return 'Thickness';
});

const toggles = computed(() => {
    const tool = annotator.activeTool.value;
    const result: { key: 'fill' | 'background' | 'shadow'; label: string }[] = [];

    if (tool === 'rectangle' || tool === 'ellipse') result.push({ key: 'fill', label: 'Fill' });
    if (tool === 'text') result.push({ key: 'background', label: 'Background' });

    if (['rectangle', 'ellipse', 'arrow', 'line', 'freehand', 'text', 'step'].includes(tool)) {
        result.push({ key: 'shadow', label: 'Shadow' });
    }

    return result;
});

const hasToggles = computed(() => toggles.value.length > 0);

const isVertical = computed(() => side === 'left' || side === 'right');

const separatorOrientation = computed(() => (isVertical.value ? 'horizontal' : 'vertical'));

const separatorClass = computed(() => (isVertical.value ? 'my-1' : 'mx-1 my-1.5'));

const handleColor = (color: string) => {
    annotator.setToolOption('color', color);
};

const handleSize = (size: number | undefined) => {
    if (size !== undefined) annotator.setToolOption('size', size);
};
</script>
