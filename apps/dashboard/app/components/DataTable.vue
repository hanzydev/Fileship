<template>
    <template v-if="isMobile && $slots.card">
        <div v-if="loading" class="flex flex-col gap-2">
            <UiCard v-for="i in 6" :key="i" size="sm">
                <UiCardContent class="flex items-center gap-3">
                    <UiSkeleton class="size-9 rounded-full" />
                    <div class="flex flex-1 flex-col gap-2">
                        <UiSkeleton class="h-4 w-1/2" />
                        <UiSkeleton class="h-3 w-1/3" />
                    </div>
                </UiCardContent>
            </UiCard>
        </div>
        <VirtualList
            v-else
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
    </template>
    <UiCard v-else class="gap-0 py-0">
        <div ref="container">
            <UiTable
                class="table-fixed [&_td:first-child]:pl-4 [&_td:last-child]:pr-4 [&_th:first-child]:pl-4 [&_th:last-child]:pr-4"
            >
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
                                @click="handleSort(column.key)"
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
                <UiTableBody v-if="loading">
                    <UiTableRow v-for="i in 8" :key="i" class="hover:bg-transparent">
                        <UiTableCell
                            v-for="column in columns"
                            :key="column.key"
                            :class="column.class"
                        >
                            <div class="flex h-9 items-center">
                                <UiSkeleton v-if="column.label" class="h-4 w-full max-w-32" />
                            </div>
                        </UiTableCell>
                    </UiTableRow>
                </UiTableBody>
                <UiTableBody v-else>
                    <tr v-if="paddingTop" :style="{ height: `${paddingTop}px` }" />
                    <UiTableRow
                        v-for="row in virtualizer.getVirtualItems()"
                        :key="itemKey(items[row.index]!)"
                        :ref="(el) => virtualizer.measureElement((el as any)?.$el ?? null)"
                        :data-index="row.index"
                        :data-state="selected?.includes(itemKey(items[row.index]!)) && 'selected'"
                        @click="emit('rowClick', items[row.index]!, $event)"
                    >
                        <UiTableCell
                            v-for="column in columns"
                            :key="column.key"
                            :class="column.class"
                        >
                            <slot :name="`cell-${column.key}`" :item="items[row.index]!">
                                {{ (items[row.index] as any)[column.key] }}
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
</template>

<script setup lang="ts" generic="T">
import { useWindowVirtualizer } from '@tanstack/vue-virtual';

export type DataTableColumn = {
    key: string;
    label: string;
    class?: string;
    sortable?: boolean;
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

const isMobile = useMediaQuery('(max-width: 767px)');
const container = useTemplateRef('container');
const scrollMargin = ref(0);

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
    if (!isMobile.value && last && last.index >= items.length - 10) emit('reachEnd');
});

const handleSort = (key: string) => {
    sort.value = sort.value?.key === key ? { key, desc: !sort.value.desc } : { key, desc: false };
};

useResizeObserver(import.meta.client ? document.body : undefined, () => {
    if (container.value) {
        scrollMargin.value =
            container.value.querySelector('tbody')!.getBoundingClientRect().top + window.scrollY;
    }
});
</script>
