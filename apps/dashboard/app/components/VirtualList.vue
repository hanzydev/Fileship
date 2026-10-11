<template>
    <div
        ref="container"
        class="relative w-full"
        :style="{ height: `${virtualizer.getTotalSize()}px` }"
    >
        <div
            v-for="row in virtualizer.getVirtualItems()"
            :key="row.key as never"
            :ref="(el) => virtualizer.measureElement(el as Element)"
            :data-index="row.index"
            class="absolute top-0 left-0 grid w-full"
            :style="{
                transform: `translateY(${row.start - virtualizer.options.scrollMargin}px)`,
                gridTemplateColumns: `repeat(${columns}, minmax(0, 1fr))`,
                gap: `${gap}px`,
                paddingBottom: `${gap}px`,
            }"
        >
            <template v-for="item in rows[row.index]" :key="itemKey(item)">
                <slot :item="item" />
            </template>
        </div>
    </div>
</template>

<script setup lang="ts" generic="T">
import { useWindowVirtualizer } from '@tanstack/vue-virtual';

const {
    items,
    itemKey,
    estimateSize = 64,
    minItemWidth,
    gap = 8,
} = defineProps<{
    items: T[];
    itemKey: (item: T) => string;
    estimateSize?: number;
    minItemWidth?: number;
    gap?: number;
}>();

const container = useTemplateRef('container');
const { width } = useElementSize(container);
const scrollMargin = ref(0);

const columns = computed(() =>
    minItemWidth ? Math.max(1, Math.floor((width.value + gap) / (minItemWidth + gap))) : 1,
);

const rows = computed(() => {
    const result: T[][] = [];
    for (let i = 0; i < items.length; i += columns.value) {
        result.push(items.slice(i, i + columns.value));
    }
    return result;
});

const virtualizer = useWindowVirtualizer(
    computed(() => ({
        count: rows.value.length,
        estimateSize: () => estimateSize + gap,
        overscan: 4,
        scrollMargin: scrollMargin.value,
    })),
);

watch(columns, () => virtualizer.value.measure());

useResizeObserver(import.meta.client ? document.body : undefined, () => {
    scrollMargin.value = container.value!.getBoundingClientRect().top + window.scrollY;
});
</script>
