<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ title }}</UiCardTitle>
            <UiCardDescription>{{ description }}</UiCardDescription>
        </UiCardHeader>
        <UiCardContent class="flex flex-col gap-6">
            <UiSkeleton
                v-if="isLoading"
                class="mx-auto aspect-square w-full max-w-52 rounded-full"
            />
            <EmptyState v-else-if="!items.length" :icon="IChartPie" title="No data yet" />
            <template v-else>
                <UiChartContainer :config="config" class="mx-auto aspect-square max-h-52">
                    <VisSingleContainer :data="data">
                        <VisDonut
                            :value="(d: Datum) => d.value"
                            :color="(d: Datum) => d.fill"
                            :arc-width="28"
                            :pad-angle="0.02"
                            :corner-radius="4"
                        />
                        <VisTooltip :triggers="{ [Donut.selectors.segment]: tooltipTemplate! }" />
                    </VisSingleContainer>
                </UiChartContainer>
                <div class="flex flex-col gap-2">
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
            </template>
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
}>();

const data = computed<Datum[]>(() =>
    [...items]
        .sort((a, b) => b.value - a.value)
        .map((item, index) => ({ ...item, fill: `var(--chart-${(index % 5) + 1})` })),
);

const config = { value: { label: valueLabel } };

const tooltipTemplate = componentToString(config, UiChartTooltipContent, { labelKey: 'label' });
</script>
