<template>
    <div
        flex="~ items-center justify-between gap4"
        wfull
        rounded-xl
        bg-fs-overlay-2
        p2
        ring="1 fs-overlay-4"
    >
        <div grid="~ cols-2 gap2">
            <UiButton
                icon="heroicons-solid:chevron-double-left"
                h8
                w8
                p0="!"
                alignment="center"
                aria-label="First page"
                :disabled="isFirstPage"
                @click="currentPage = 1"
            />
            <UiButton
                icon="heroicons-solid:chevron-left"
                h8
                w8
                p0="!"
                alignment="center"
                aria-label="Previous page"
                :disabled="isFirstPage"
                @click="prev"
            />
        </div>

        <div flex="~ gap2 items-center">
            <template v-for="(page, index) in calculatedRange">
                <template v-if="page === '...'">...</template>
                <UiButton
                    v-else
                    :key="index"
                    alignment="center"
                    h8
                    px3="!"
                    py0="!"
                    :variant="currentPage === page ? 'accent' : 'primary'"
                    @click="currentPage = page"
                >
                    {{ page }}
                </UiButton>
            </template>
        </div>
        <div grid="~ cols-2 gap2">
            <UiButton
                icon="heroicons-solid:chevron-right"
                h8
                w8
                p0="!"
                alignment="center"
                aria-label="Next page"
                :disabled="isLastPage"
                @click="next"
            />
            <UiButton
                icon="heroicons-solid:chevron-double-right"
                h8
                w8
                p0="!"
                alignment="center"
                aria-label="Last page"
                :disabled="isLastPage"
                @click="currentPage = pageCount"
            />
        </div>
    </div>
</template>

<script setup lang="ts">
const props = defineProps<{
    itemCount: number;
    itemsPerPage: number;
}>();

const { itemCount, itemsPerPage } = toRefs(props);

const currentPage = defineModel<number>({ required: true });

const { pageCount, isFirstPage, isLastPage, prev, next } = useOffsetPagination({
    total: itemCount,
    pageSize: ref(itemsPerPage.value),
    page: currentPage,
});

const calculatedRange = computed(() => {
    const range = [];
    const rangeWithDots = [];
    const delta = 2;

    for (let i = 1; i <= pageCount.value; i++) {
        if (
            i == 1 ||
            i == pageCount.value ||
            (i >= currentPage.value - delta && i < currentPage.value + delta + 1)
        ) {
            range.push(i);
        }
    }

    let lastIndex: number | null = null;

    for (const i of range) {
        if (lastIndex) {
            if (i - lastIndex === 2) rangeWithDots.push(lastIndex + 1);
            else if (i - lastIndex !== 1) rangeWithDots.push('...');
        }

        rangeWithDots.push(i);
        lastIndex = i;
    }

    return rangeWithDots as (number | '...')[];
});
</script>
