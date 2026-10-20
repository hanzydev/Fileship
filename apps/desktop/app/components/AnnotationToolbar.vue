<template>
    <div :class="cn('flex items-center gap-1', isVertical ? 'flex-col' : 'flex-wrap')">
        <UiTooltipProvider disable-hoverable-content>
            <slot name="start" />

            <UiTooltip v-for="tool in tools" :key="tool.value">
                <UiTooltipTrigger as-child>
                    <UiButton
                        :variant="annotator.tool.value === tool.value ? 'secondary' : 'ghost'"
                        size="icon-sm"
                        :aria-label="$t(tool.label)"
                        :aria-pressed="annotator.tool.value === tool.value"
                        @click="annotator.setTool(tool.value)"
                    >
                        <component :is="tool.icon" />
                    </UiButton>
                </UiTooltipTrigger>
                <UiTooltipContent :side="side">
                    {{ $t(tool.label) }}
                    <UiKbd class="ml-1">
                        {{ ANNOTATION_SHORTCUTS[tool.value].toUpperCase() }}
                    </UiKbd>
                </UiTooltipContent>
            </UiTooltip>

            <UiSeparator :orientation="separatorOrientation" :class="separatorClass" />

            <UiPopover v-if="isColored">
                <UiPopoverTrigger as-child>
                    <UiButton variant="ghost" size="icon-sm" :aria-label="$t('annotate.color')">
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
                            :aria-label="$t('annotate.customColor')"
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
                    <UiButton variant="ghost" size="icon-sm" :aria-label="$t('annotate.emoji')">
                        <span class="text-base leading-none">
                            {{ annotator.options.value.emoji }}
                        </span>
                    </UiButton>
                </UiPopoverTrigger>
                <UiPopoverContent
                    class="w-72 shadow-md data-closed:zoom-out-100! data-open:zoom-in-100! data-[side=bottom]:slide-in-from-top-0! data-[side=left]:slide-in-from-right-0! data-[side=right]:slide-in-from-left-0! data-[side=top]:slide-in-from-bottom-0!"
                    align="center"
                    :side="side"
                >
                    <div
                        class="grid grid-cols-8 gap-1 font-['Segoe_UI_Emoji','Apple_Color_Emoji',sans-serif]"
                    >
                        <button
                            v-for="emoji in EMOJIS"
                            :key="emoji"
                            type="button"
                            :aria-pressed="annotator.options.value.emoji === emoji"
                            class="flex size-8 items-center justify-center rounded-full text-lg leading-none transition-colors outline-none hover:bg-muted focus-visible:ring-3 focus-visible:ring-ring/30 aria-pressed:bg-muted"
                            @click="annotator.setOption('emoji', emoji)"
                        >
                            {{ emoji }}
                        </button>
                    </div>
                </UiPopoverContent>
            </UiPopover>

            <UiPopover v-if="range || hasToggles">
                <UiPopoverTrigger as-child>
                    <UiButton variant="ghost" size="icon-sm" :aria-label="$t('annotate.options')">
                        <ISlidersHorizontal />
                    </UiButton>
                </UiPopoverTrigger>
                <UiPopoverContent class="w-64" align="center" :side="side">
                    <UiFieldGroup>
                        <UiField v-if="range">
                            <UiFieldLabel>
                                {{ $t(sizeLabel) }} · {{ annotator.current.value.size }}
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
                                    {{ $t(toggle.label) }}
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
                        :aria-label="$t('annotate.undo')"
                        :disabled="!annotator.canUndo.value"
                        @click="annotator.undo"
                    >
                        <IUndo2 />
                    </UiButton>
                </UiTooltipTrigger>
                <UiTooltipContent :side="side">
                    {{ $t('annotate.undo') }}
                    <UiKbd class="ml-1">{{ KEYS.undo }}</UiKbd>
                </UiTooltipContent>
            </UiTooltip>
            <UiTooltip>
                <UiTooltipTrigger as-child>
                    <UiButton
                        variant="ghost"
                        size="icon-sm"
                        :aria-label="$t('annotate.redo')"
                        :disabled="!annotator.canRedo.value"
                        @click="annotator.redo"
                    >
                        <IRedo2 />
                    </UiButton>
                </UiTooltipTrigger>
                <UiTooltipContent :side="side">
                    {{ $t('annotate.redo') }}
                    <UiKbd class="ml-1">{{ KEYS.redo }}</UiKbd>
                </UiTooltipContent>
            </UiTooltip>
            <UiTooltip>
                <UiTooltipTrigger as-child>
                    <UiButton
                        variant="ghost"
                        size="icon-sm"
                        :aria-label="$t('annotate.delete')"
                        :disabled="!annotator.selectedId.value"
                        @click="annotator.removeSelected"
                    >
                        <ITrash2 />
                    </UiButton>
                </UiTooltipTrigger>
                <UiTooltipContent :side="side">
                    {{ $t('annotate.delete') }}
                    <UiKbd class="ml-1">{{ KEYS.delete }}</UiKbd>
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

const { annotator, side = 'bottom' } = defineProps<{
    annotator: Annotator;
    side?: 'top' | 'bottom' | 'left' | 'right';
}>();

const KEYS = { undo: 'Ctrl Z', redo: 'Ctrl Y', delete: 'Del' };

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

const TOOL_ICONS: Record<AnnotationTool, Component> = {
    select: IMousePointer2,
    rectangle: ISquare,
    ellipse: ICircle,
    arrow: IMoveUpRight,
    line: IMinus,
    freehand: IPencil,
    highlight: IHighlighter,
    text: IType,
    step: IListOrdered,
    blur: IDroplets,
    pixelate: IGrid3x3,
    eraser: IEraser,
    emoji: ISmile,
};

const tools = ANNOTATION_TOOLS.map((value) => ({
    value,
    label: `annotate.tools.${value}`,
    icon: TOOL_ICONS[value],
}));

const isColored = computed(() => COLORED_TOOLS.includes(annotator.activeTool.value));

const range = computed(() => SIZE_RANGES[annotator.activeTool.value]);

const sizeLabel = computed(() => {
    const tool = annotator.activeTool.value;
    if (tool === 'text' || tool === 'emoji') return 'annotate.size';
    if (tool === 'step') return 'annotate.radius';
    if (tool === 'blur' || tool === 'pixelate') return 'annotate.strength';
    return 'annotate.thickness';
});

const toggles = computed(() => {
    const tool = annotator.activeTool.value;
    const result: { key: 'fill' | 'background' | 'shadow'; label: string }[] = [];

    if (tool === 'rectangle' || tool === 'ellipse')
        result.push({ key: 'fill', label: 'annotate.fill' });
    if (tool === 'text') result.push({ key: 'background', label: 'annotate.background' });

    if (['rectangle', 'ellipse', 'arrow', 'line', 'freehand', 'text', 'step'].includes(tool)) {
        result.push({ key: 'shadow', label: 'annotate.shadow' });
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
