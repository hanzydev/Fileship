<template>
    <div class="w-full">
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
        <div
            v-if="loading"
            class="grid w-full"
            :style="{ gridTemplateColumns: `repeat(${columns}, minmax(0, 1fr))`, gap: `${gap}px` }"
        >
            <template v-for="i in columns > 1 ? columns : 3" :key="i">
                <slot name="skeleton" />
            </template>
        </div>
    </div>
</template>

<script setup lang="ts" generic="T">
import type { Virtualizer } from '@tanstack/vue-virtual';
import { useVirtualizer, useWindowVirtualizer } from '@tanstack/vue-virtual';

const {
    items,
    itemKey,
    estimateSize = 64,
    minItemWidth,
    gap = 8,
    loading,
    contained,
} = defineProps<{
    items: T[];
    itemKey: (item: T) => string;
    estimateSize?: number;
    minItemWidth?: number;
    gap?: number;
    loading?: boolean;
    contained?: boolean;
}>();

const emit = defineEmits<{
    reachEnd: [];
}>();

const container = useTemplateRef('container');
const scroller = shallowRef<HTMLElement | null>(null);
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

const options = computed(() => ({
    count: rows.value.length,
    estimateSize: () => estimateSize + gap,
    overscan: 4,
    scrollMargin: scrollMargin.value,
}));

const virtualizer = (
    contained
        ? useVirtualizer(
              computed(() => ({ ...options.value, getScrollElement: () => scroller.value })),
          )
        : useWindowVirtualizer(options)
) as Ref<Virtualizer<any, Element>>;

const getScrollParent = (element: HTMLElement) => {
    let parent = element.parentElement;

    while (parent && !/auto|scroll/.test(getComputedStyle(parent).overflowY)) {
        parent = parent.parentElement;
    }

    return parent ?? document.documentElement;
};

const updateScrollMargin = () => {
    if (!container.value) return;

    if (!contained) {
        scrollMargin.value = container.value.getBoundingClientRect().top + window.scrollY;
    } else if (scroller.value) {
        scrollMargin.value =
            container.value.getBoundingClientRect().top -
            scroller.value.getBoundingClientRect().top +
            scroller.value.scrollTop;
    }
};

watch(columns, () => virtualizer.value.measure());

watchEffect(() => {
    const last = virtualizer.value.getVirtualItems().at(-1);
    if (last && last.index >= rows.value.length - 3) emit('reachEnd');
});

useResizeObserver(
    computed(() => (import.meta.client ? (contained ? scroller.value : document.body) : null)),
    updateScrollMargin,
);

onMounted(() => {
    if (contained) scroller.value = getScrollParent(container.value!);
    updateScrollMargin();
});
</script>
