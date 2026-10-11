<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ title }}</UiCardTitle>
            <UiCardDescription>{{ description }}</UiCardDescription>
        </UiCardHeader>
        <UiCardContent :class="cn('flex flex-col gap-6', fill && 'flex-1')">
            <EmptyState v-if="!isLoading && !items.length" :icon="IChartPie" title="No data yet" />
            <div v-else :class="cn('relative', fill ? 'min-h-32 flex-1' : 'h-52')">
                <UiSkeleton
                    v-if="isLoading"
                    class="absolute inset-y-0 left-1/2 aspect-square -translate-x-1/2 rounded-full"
                />
                <UiChartContainer
                    v-else
                    :config="config"
                    class="absolute inset-y-0 left-1/2 aspect-square w-auto -translate-x-1/2"
                >
                    <VisSingleContainer :data="data">
                        <VisDonut
                            :value="(d: Datum) => d.value"
                            :color="(d: Datum) => d.fill"
                            :arc-width="28"
                        />
                        <VisTooltip :triggers="{ [Donut.selectors.segment]: tooltip }" />
                    </VisSingleContainer>
                </UiChartContainer>
            </div>
            <div v-if="!isLoading && items.length" class="flex flex-col gap-2">
                <div
                    v-for="item in data.slice(0, 6)"
                    :key="item.label"
                    class="flex items-center gap-2 text-sm"
                >
                    <span
                        class="size-2.5 shrink-0 rounded-full"
                        :style="{ background: item.fill }"
                    />
                    <UiAvatar v-if="item.avatar !== undefined" class="size-5">
                        <UiAvatarImage v-if="item.avatar" :src="item.avatar" />
                        <UiAvatarFallback>{{ item.label[0]!.toUpperCase() }}</UiAvatarFallback>
                    </UiAvatar>
                    <span class="flex-1 truncate">{{ item.label }}</span>
                    <span class="text-muted-foreground">
                        {{ item.formatted ?? item.value }}
                    </span>
                </div>
            </div>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
import { Donut } from '@unovis/ts';
import { VisDonut, VisSingleContainer, VisTooltip } from '@unovis/vue';

import { IChartPie, UiChartTooltipContent } from '#components';
import { componentToString } from '~/components/ui/chart';

export type DonutItem = {
    label: string;
    value: number;
    formatted?: string;
    avatar?: string | null;
};

type Datum = DonutItem & { fill: string };

const { items, valueLabel } = defineProps<{
    title: string;
    description: string;
    items: DonutItem[];
    valueLabel: string;
    isLoading?: boolean;
    fill?: boolean;
}>();

const data = computed<Datum[]>(() =>
    [...items]
        .sort((a, b) => b.value - a.value)
        .map((item, index) => ({ ...item, fill: `var(--chart-${(index % 5) + 1})` })),
);

const config = { value: { label: valueLabel } };

const tooltipTemplate = componentToString(config, UiChartTooltipContent, { labelKey: 'label' })!;

const tooltip = (d: { data: Datum }, x: number) =>
    tooltipTemplate({ ...d, data: { ...d.data, value: d.data.formatted ?? d.data.value } }, x);
</script>
