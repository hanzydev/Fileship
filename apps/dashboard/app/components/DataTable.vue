<template>
    <div ref="root" class="min-w-0">
        <template v-if="loading">
            <div v-if="$slots.card" class="flex flex-col gap-2 md:hidden">
                <template v-for="i in 6" :key="i">
                    <slot name="card-skeleton">
                        <ListCardSkeleton media="size-8" />
                    </slot>
                </template>
            </div>
            <UiCard :class="cn('gap-0 py-0', $slots.card && 'hidden md:flex')">
                <UiTable :class="tableClass">
                    <UiTableHeader>
                        <UiTableRow>
                            <UiTableHead
                                v-for="column in columns"
                                :key="column.key"
                                :class="column.class"
                            >
                                {{ column.label }}
                            </UiTableHead>
                        </UiTableRow>
                    </UiTableHeader>
                    <UiTableBody>
                        <UiTableRow v-for="i in 8" :key="i" class="hover:bg-transparent">
                            <UiTableCell
                                v-for="column in columns"
                                :key="column.key"
                                :class="column.class"
                            >
                                <div class="flex h-9 items-center">
                                    <UiSkeleton
                                        v-if="column.label"
                                        class="h-lh w-full max-w-32 rounded-full"
                                    />
                                </div>
                            </UiTableCell>
                        </UiTableRow>
                    </UiTableBody>
                </UiTable>
            </UiCard>
        </template>
        <VirtualList
            v-else-if="isCards"
            :items="items"
            :item-key="itemKey"
            :estimate-size="cardEstimateSize"
            :loading="loadingMore"
            @reach-end="emit('reachEnd')"
        >
            <template #default="{ item }">
                <slot name="card" :item="item" />
            </template>
        </VirtualList>
        <UiCard v-else class="gap-0 py-0">
            <div ref="container">
                <UiTable :class="tableClass">
                    <UiTableHeader>
                        <UiTableRow>
                            <UiTableHead
                                v-for="column in columns"
                                :key="column.key"
                                :class="column.class"
                            >
                                <UiButton
                                    v-if="column.sortable"
                                    variant="ghost"
                                    size="sm"
                                    class="-ml-2"
                                    @click="handleSort(column)"
                                >
                                    {{ column.label }}
                                    <IArrowUp v-if="sort?.key === column.key && !sort.desc" />
                                    <IArrowDown v-else-if="sort?.key === column.key" />
                                    <IArrowUpDown v-else class="opacity-50" />
                                </UiButton>
                                <template v-else>{{ column.label }}</template>
                            </UiTableHead>
                        </UiTableRow>
                    </UiTableHeader>
                    <UiTableBody>
                        <tr v-if="paddingTop" :style="{ height: `${paddingTop}px` }" />
                        <UiTableRow
                            v-for="row in virtualizer.getVirtualItems()"
                            :key="itemKey(items[row.index]!)"
                            :ref="(el) => virtualizer.measureElement((el as any)?.$el ?? null)"
                            :data-index="row.index"
                            :data-state="
                                selected?.includes(itemKey(items[row.index]!)) && 'selected'
                            "
                            @click="emit('rowClick', items[row.index]!, $event)"
                        >
                            <UiTableCell
                                v-for="column in columns"
                                :key="column.key"
                                :class="column.class"
                            >
                                <slot :name="`cell-${column.key}`" :item="items[row.index]!">
                                    <span class="block truncate">
                                        {{ (items[row.index] as any)[column.key] }}
                                    </span>
                                </slot>
                            </UiTableCell>
                        </UiTableRow>
                        <tr v-if="paddingBottom" :style="{ height: `${paddingBottom}px` }" />
                        <UiTableRow v-if="loadingMore" class="hover:bg-transparent">
                            <UiTableCell :colspan="columns.length">
                                <div class="flex justify-center py-2">
                                    <UiSpinner class="text-muted-foreground" />
                                </div>
                            </UiTableCell>
                        </UiTableRow>
                    </UiTableBody>
                </UiTable>
            </div>
        </UiCard>
    </div>
</template>

<script setup lang="ts" generic="T">
import { useWindowVirtualizer } from '@tanstack/vue-virtual';

export type DataTableColumn = {
    key: string;
    label: string;
    class?: string;
    sortable?: boolean;
    descFirst?: boolean;
};

export type DataTableSort = {
    key: string;
    desc: boolean;
};

const {
    items,
    itemKey,
    columns,
    selected,
    loading,
    loadingMore,
    estimateSize = 53,
    cardEstimateSize = 96,
} = defineProps<{
    items: T[];
    itemKey: (item: T) => string;
    columns: DataTableColumn[];
    selected?: string[];
    loading?: boolean;
    loadingMore?: boolean;
    estimateSize?: number;
    cardEstimateSize?: number;
}>();

const emit = defineEmits<{
    rowClick: [item: T, event: MouseEvent];
    reachEnd: [];
}>();

const sort = defineModel<DataTableSort>('sort');

const slots = useSlots();
const isViewportMobile = useMediaQuery('(max-width: 767px)');
const root = useTemplateRef('root');
const container = useTemplateRef('container');
const { width } = useElementSize(root);
const scrollMargin = ref(0);

const tableClass =
    'table-fixed [&_td:first-child]:pl-4 [&_td:last-child]:pr-4 [&_th:first-child]:pl-4 [&_th:last-child]:pr-4';

const isCards = computed(
    () => !!slots.card && (width.value ? width.value < 640 : isViewportMobile.value),
);

const virtualizer = useWindowVirtualizer(
    computed(() => ({
        count: items.length,
        estimateSize: () => estimateSize,
        overscan: 8,
        scrollMargin: scrollMargin.value,
    })),
);

const paddingTop = computed(() => {
    const first = virtualizer.value.getVirtualItems()[0];
    return first ? first.start - scrollMargin.value : 0;
});

const paddingBottom = computed(() => {
    const last = virtualizer.value.getVirtualItems().at(-1);
    return last ? virtualizer.value.getTotalSize() - (last.end - scrollMargin.value) : 0;
});

watchEffect(() => {
    const last = virtualizer.value.getVirtualItems().at(-1);
    if (!isCards.value && last && last.index >= items.length - 10) emit('reachEnd');
});

const handleSort = (column: DataTableColumn) => {
    sort.value =
        sort.value?.key === column.key
            ? { key: column.key, desc: !sort.value.desc }
            : { key: column.key, desc: !!column.descFirst };
};

useResizeObserver(import.meta.client ? document.body : undefined, () => {
    if (container.value) {
        scrollMargin.value =
            container.value.querySelector('tbody')!.getBoundingClientRect().top + window.scrollY;
    }
});
</script>
