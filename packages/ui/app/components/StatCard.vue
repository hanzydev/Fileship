<template>
    <UiCard>
        <UiCardHeader>
            <UiCardDescription>{{ title }}</UiCardDescription>
            <UiCardTitle class="text-3xl font-semibold tabular-nums">
                <UiSkeleton v-if="isLoading" class="h-lh w-24 rounded-full" />
                <template v-else>
                    {{ typeof data === 'number' ? numberFormat.format(data) : data }}
                </template>
            </UiCardTitle>
            <UiCardAction>
                <UiBadge v-if="!isLoading && growth" variant="outline">
                    <ITrendingUp v-if="growth! > 0" />
                    <ITrendingDown v-else />
                    {{ growth! > 0 ? '+' : '' }}{{ growth }}%
                </UiBadge>
                <component :is="icon" v-else class="size-5 text-muted-foreground" />
            </UiCardAction>
        </UiCardHeader>
        <UiCardFooter>
            <TextBlock variant="muted">{{ legend }}</TextBlock>
        </UiCardFooter>
    </UiCard>
</template>

<script setup lang="ts">
const { data } = defineProps<{
    title: string;
    legend: string;
    icon: any;
    growth?: number;
    data?: number | string;
}>();

const numberFormat = new Intl.NumberFormat('en-US');

const isLoading = computed(() => data === undefined);
</script>
