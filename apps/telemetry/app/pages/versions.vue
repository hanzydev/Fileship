<template>
    <Head>
        <Title>Versions</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader title="Versions" description="Fileship versions running on active instances.">
            <UiTabs v-model="range">
                <UiTabsList>
                    <UiTabsTrigger v-for="option in STAT_RANGES" :key="option" :value="option">
                        {{ RANGE_LABELS[option] }}
                    </UiTabsTrigger>
                </UiTabsList>
            </UiTabs>
        </PageHeader>

        <EmptyState
            v-if="loadError"
            :icon="ICircleAlert"
            title="Couldn't load the statistics"
            :description="getErrorMessage(loadError)"
        >
            <UiButton variant="outline" @click="handleRetry">Try again</UiButton>
        </EmptyState>

        <template v-else>
            <UiCard>
                <UiCardHeader>
                    <UiCardTitle>Adoption</UiCardTitle>
                    <UiCardDescription>
                        Active instances per version, the {{ TOP_VERSIONS }} most used ones and the
                        rest as Other
                    </UiCardDescription>
                </UiCardHeader>
                <UiCardContent>
                    <UiSkeleton v-if="!series" class="h-72 w-full" />
                    <EmptyState
                        v-else-if="!data.length"
                        :icon="IChartArea"
                        title="No data yet"
                        class="h-72 py-0"
                    />
                    <UiChartContainer v-else :config="config" class="aspect-auto h-72" cursor>
                        <VisXYContainer :data="data" :y-domain="[0, undefined]">
                            <VisArea
                                :x="(d: Datum) => d.index"
                                :y="accessors"
                                :color="colors"
                                :opacity="0.6"
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
                            />
                            <VisTooltip />
                            <VisCrosshair :template="tooltip" :color="colors" />
                        </VisXYContainer>
                    </UiChartContainer>
                </UiCardContent>
            </UiCard>

            <StatsDonut
                title="Current versions"
                :description="`Versions of ${pluralize(latest?.instances ?? 0, 'active instance')}`"
                value-label="Instances"
                :is-loading="latest === undefined"
                :items="toDonutItems(latest?.versions)"
            />
        </template>
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

import { VisArea, VisAxis, VisCrosshair, VisTooltip, VisXYContainer } from '@unovis/vue';

import { IChartArea, ICircleAlert, UiChartTooltipContent } from '#components';
import { type ChartConfig, componentToString } from '#layers/ui/app/components/ui/chart';

type Datum = { index: number; label: string } & Record<string, number | string>;

const TOP_VERSIONS = 5;

const range = useQueryState<(typeof STAT_RANGES)[number]>('range', '90d');

const handleUnauthorized = useUnauthorizedHandler();

const {
    data: latest,
    error: latestError,
    refresh: refreshLatest,
} = useFetch('/api/stats/latest', { server: false, onResponseError: handleUnauthorized });

const {
    data: series,
    error: seriesError,
    refresh: refreshSeries,
} = useFetch('/api/stats/series', {
    query: { range },
    server: false,
    onResponseError: handleUnauthorized,
});

const loadError = computed(() => latestError.value || seriesError.value);

const handleRetry = () => Promise.all([refreshLatest(), refreshSeries()]);

const config = reactive<ChartConfig>({});

const topVersions = computed(() => {
    const totals: Record<string, number> = {};

    for (const stat of series.value ?? []) {
        for (const [version, count] of Object.entries(stat.versions)) {
            totals[version] = (totals[version] ?? 0) + count;
        }
    }

    return Object.entries(totals)
        .sort((a, b) => b[1] - a[1])
        .slice(0, TOP_VERSIONS)
        .map(([version]) => version);
});

const keys = computed(() => [...topVersions.value.map((_, index) => `v${index}`), 'other']);

const accessors = computed(() => keys.value.map((key) => (d: Datum) => d[key] as number));

const colors = computed(() =>
    keys.value.map((key, index) =>
        key === 'other' ? 'var(--muted-foreground)' : `var(--chart-${index + 1})`,
    ),
);

const data = computed<Datum[]>(() =>
    (series.value ?? []).map((stat, index) => {
        const datum: Datum = { index, label: dayjs(stat.day).format('MMM D'), other: 0 };

        for (const [version, count] of Object.entries(stat.versions)) {
            const position = topVersions.value.indexOf(version);
            const key = position === -1 ? 'other' : `v${position}`;
            datum[key] = ((datum[key] as number | undefined) ?? 0) + count;
        }

        for (const key of keys.value) datum[key] ??= 0;

        return datum;
    }),
);

watchEffect(() => {
    for (let index = 0; index < TOP_VERSIONS; index++) {
        config[`v${index}`] = {
            label: topVersions.value[index] ?? '',
            color: `var(--chart-${index + 1})`,
        };
    }

    config.other = { label: 'Other', color: 'var(--muted-foreground)' };
});

const tooltip = componentToString(config, UiChartTooltipContent, { labelKey: 'label' });

definePageMeta({
    layout: 'panel',
});
</script>
