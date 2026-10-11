<template>
    <UiChartContainer :config="config" class="aspect-auto" cursor>
        <VisXYContainer :data="data" :margin="{ left: -12 }" :y-domain="[0, undefined]">
            <VisArea
                :x="(_: Datum, i: number) => i"
                :y="(d: Datum) => d.views"
                color="var(--chart-2)"
                :opacity="0.3"
            />
            <VisLine
                :x="(_: Datum, i: number) => i"
                :y="(d: Datum) => d.views"
                color="var(--chart-2)"
                :line-width="2"
            />
            <VisAxis
                type="x"
                :x="(_: Datum, i: number) => i"
                :tick-line="false"
                :domain-line="false"
                :grid-line="false"
                :num-ticks="numTicks"
                :tick-format="(i: number) => tickFormat(data[i]?.label ?? '')"
            />
            <VisAxis type="y" :num-ticks="4" :tick-line="false" :domain-line="false" />
            <VisTooltip />
            <VisCrosshair :template="tooltip" color="var(--chart-2)" />
        </VisXYContainer>
    </UiChartContainer>
</template>

<script setup lang="ts">
import { VisArea, VisAxis, VisCrosshair, VisLine, VisTooltip, VisXYContainer } from '@unovis/vue';

import { UiChartTooltipContent } from '#components';
import { componentToString } from '~/components/ui/chart';

type Datum = { label: string; views: number };

const { tickFormat = (label: string) => label } = defineProps<{
    data: Datum[];
    numTicks?: number;
    tickFormat?: (label: string) => string;
}>();

const config = { views: { label: 'Views', color: 'var(--chart-2)' } };

const tooltip = componentToString(config, UiChartTooltipContent, { labelKey: 'label' });
</script>
