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
                    {{ percentFormat.format(growth! / 100) }}
                </UiBadge>
                <component :is="icon" v-else class="size-5 text-muted-foreground" />
            </UiCardAction>
        </UiCardHeader>
        <UiCardFooter :class="progress !== undefined && 'flex-col items-stretch gap-2'">
            <UiProgress
                v-if="progress !== undefined"
                :model-value="Math.min(progress, 100)"
                :aria-label="title"
                :class="
                    cn('h-1.5', progress >= 90 && '*:data-[slot=progress-indicator]:bg-destructive')
                "
            />
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
    progress?: number;
}>();

const { locale } = useI18n();

const numberFormat = computed(() => new Intl.NumberFormat(locale.value));
const percentFormat = computed(
    () =>
        new Intl.NumberFormat(locale.value, {
            style: 'percent',
            signDisplay: 'exceptZero',
            maximumFractionDigits: 2,
        }),
);

const isLoading = computed(() => data === undefined);
</script>
