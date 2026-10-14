<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ title }}</UiCardTitle>
            <UiCardDescription>{{ description }}</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiSkeleton v-if="isLoading" class="h-56 w-full" />
            <EmptyState
                v-else-if="!data.length"
                :icon="IChartArea"
                title="No data yet"
                class="h-56 py-0"
            />
            <UiChartContainer v-else :config="config" class="aspect-auto h-56" cursor>
                <VisXYContainer :data="data" :y-domain="[0, undefined]">
                    <VisArea
                        :x="(d: Datum) => d.index"
                        :y="(d: Datum) => d.value"
                        :color="color"
                        :opacity="0.3"
                    />
                    <VisLine
                        :x="(d: Datum) => d.index"
                        :y="(d: Datum) => d.value"
                        :color="color"
                        :line-width="2"
                    />
                    <VisAxis
                        type="x"
                        :x="(d: Datum) => d.index"
                        :tick-line="false"
                        :domain-line="false"
                        :grid-line="false"
                        :num-ticks="6"
                        :tick-format="(i: number) => data[i]?.label ?? ''"
                    />
                    <VisAxis
                        type="y"
                        :num-ticks="4"
                        :tick-line="false"
                        :domain-line="false"
                        :tick-format="formatValue"
                    />
                    <VisTooltip />
                    <VisCrosshair :template="tooltip" :color="color" />
                </VisXYContainer>
            </UiChartContainer>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

import { VisArea, VisAxis, VisCrosshair, VisLine, VisTooltip, VisXYContainer } from '@unovis/vue';

import { IChartArea, UiChartTooltipContent } from '#components';
import { componentToString } from '#layers/ui/app/components/ui/chart';

type Datum = { index: number; label: string; value: number };

const {
    points,
    valueLabel,
    color = 'var(--chart-2)',
    format,
} = defineProps<{
    title: string;
    description: string;
    points: { day: string; value: number }[];
    valueLabel: string;
    color?: string;
    format?: (value: number) => string;
    isLoading?: boolean;
}>();

const formatValue = (value: number) => (format ?? formatNumber)(value);

const data = computed<Datum[]>(() =>
    points.map((point, index) => ({
        index,
        label: dayjs(point.day).format('MMM D'),
        value: point.value,
    })),
);

const config = computed(() => ({ value: { label: valueLabel, color } }));

const tooltipTemplate = componentToString(config.value, UiChartTooltipContent, {
    labelKey: 'label',
})!;

const tooltip = (d: Datum, x: number) => tooltipTemplate({ ...d, value: formatValue(d.value) }, x);
</script>
