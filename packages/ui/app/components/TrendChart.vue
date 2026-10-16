<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ title }}</UiCardTitle>
            <UiCardDescription>{{ description }}</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <EmptyState
                v-if="isError"
                :icon="ICircleAlert"
                :title="$t('common.loadFailed')"
                class="h-56 py-0"
            />
            <UiSkeleton v-else-if="isLoading" class="h-56 w-full" />
            <EmptyState
                v-else-if="!data.length"
                :icon="IChartArea"
                :title="$t('common.noData')"
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
                        :num-ticks="numTicks"
                        :tick-format="(i: number) => data[i]?.tick ?? ''"
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
import { VisArea, VisAxis, VisCrosshair, VisLine, VisTooltip, VisXYContainer } from '@unovis/vue';

import { IChartArea, ICircleAlert, UiChartTooltipContent } from '#components';
import { componentToString } from '#layers/ui/app/components/ui/chart';

export type TrendPoint = { label: string; value: number; tick?: string };

type Datum = { index: number; label: string; tick: string; value: number };

const { locale } = useI18n();

const numberFormat = computed(() => new Intl.NumberFormat(locale.value));

const {
    points,
    valueLabel,
    color = 'var(--chart-2)',
    format,
    numTicks = 6,
} = defineProps<{
    title: string;
    description: string;
    points: TrendPoint[];
    valueLabel: string;
    color?: string;
    format?: (value: number) => string;
    numTicks?: number;
    isLoading?: boolean;
    isError?: boolean;
}>();

const formatValue = (value: number) => (format ?? numberFormat.value.format)(value);

const data = computed<Datum[]>(() =>
    points.map((point, index) => ({
        index,
        label: point.label,
        tick: point.tick ?? point.label,
        value: point.value,
    })),
);

const config = computed(() => ({ value: { label: valueLabel, color } }));

const tooltipTemplate = componentToString(config.value, UiChartTooltipContent, {
    labelKey: 'label',
})!;

const tooltip = (d: Datum, x: number) => tooltipTemplate({ ...d, value: formatValue(d.value) }, x);
</script>
