<template>
    <Head>
        <Title>Overview</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader
            title="Overview"
            :description="`Instances that reported in the last ${ACTIVE_DAYS} days, refreshed hourly.`"
        >
            <UiTabs v-model="range">
                <UiTabsList>
                    <UiTabsTrigger v-for="option in STAT_RANGES" :key="option" :value="option">
                        {{ RANGE_LABELS[option] }}
                    </UiTabsTrigger>
                </UiTabsList>
            </UiTabs>
        </PageHeader>

        <div class="grid auto-rows-min gap-4 sm:grid-cols-2 xl:grid-cols-4">
            <StatCard v-for="card in statCards" :key="card.title" v-bind="card" />
        </div>

        <div class="grid gap-4 lg:grid-cols-2">
            <TrendChart
                v-for="chart in charts"
                :key="chart.title"
                v-bind="chart"
                :is-loading="!series"
            />
        </div>
    </div>
</template>

<script setup lang="ts">
import { IBox, IFiles, IServer, IUsers } from '#components';

const range = useQueryState<(typeof STAT_RANGES)[number]>('range', '90d');

const { data: latest } = useFetch('/api/stats/latest', { server: false });
const { data: series } = useFetch('/api/stats/series', {
    query: { range },
    server: false,
});

const statCards = computed(() => [
    {
        title: 'Instances',
        legend: 'active instances',
        icon: IServer,
        data: latest.value === undefined ? undefined : (latest.value?.instances ?? 0),
        growth: latest.value?.growth.instances ?? undefined,
    },
    {
        title: 'Users',
        legend: 'users on active instances',
        icon: IUsers,
        data: latest.value === undefined ? undefined : (latest.value?.users ?? 0),
        growth: latest.value?.growth.users ?? undefined,
    },
    {
        title: 'Files',
        legend: 'files on active instances',
        icon: IFiles,
        data: latest.value === undefined ? undefined : (latest.value?.files ?? 0),
        growth: latest.value?.growth.files ?? undefined,
    },
    {
        title: 'Storage',
        legend: 'used by files on active instances',
        icon: IBox,
        data: latest.value === undefined ? undefined : formatBytes(latest.value?.storage ?? 0),
        growth: latest.value?.growth.storage ?? undefined,
    },
]);

const charts = computed(() => [
    {
        title: 'Active instances',
        description: `Instances that reported in the ${ACTIVE_DAYS} days before each day`,
        valueLabel: 'Instances',
        color: 'var(--chart-2)',
        points: (series.value ?? []).map((stat) => ({ day: stat.day, value: stat.instances })),
    },
    {
        title: 'Users',
        description: 'Users on the active instances',
        valueLabel: 'Users',
        color: 'var(--chart-3)',
        points: (series.value ?? []).map((stat) => ({ day: stat.day, value: stat.users })),
    },
    {
        title: 'Files',
        description: 'Files on the active instances, trash excluded',
        valueLabel: 'Files',
        color: 'var(--chart-4)',
        points: (series.value ?? []).map((stat) => ({ day: stat.day, value: stat.files })),
    },
    {
        title: 'Storage',
        description: 'Size of those files, reported by Fileship 6 and later',
        valueLabel: 'Storage',
        color: 'var(--chart-1)',
        format: formatBytes,
        points: (series.value ?? []).map((stat) => ({ day: stat.day, value: stat.storage })),
    },
]);

definePageMeta({
    layout: 'panel',
});
</script>
