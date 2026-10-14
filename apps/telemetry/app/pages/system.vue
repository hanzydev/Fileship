<template>
    <Head>
        <Title>System</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader
            title="System"
            :description="`Based on ${pluralize(latest?.data.runtime ?? 0, 'active instance')} that sent system information.`"
        />

        <div class="grid auto-rows-min gap-4 sm:grid-cols-2 xl:grid-cols-4">
            <StatCard v-for="card in statCards" :key="card.title" v-bind="card" />
        </div>

        <div class="flex flex-wrap gap-4">
            <StatsDonut
                v-for="chart in charts"
                :key="chart.title"
                class="min-w-0 grow basis-full lg:basis-[calc((100%-1rem)/2)] xl:basis-[calc((100%-2rem)/3)]"
                v-bind="chart"
                value-label="Instances"
                :is-loading="latest === undefined"
            />
        </div>
    </div>
</template>

<script setup lang="ts">
import { IContainer, ICpu, IMemoryStick, IMonitor } from '#components';

const { data: latest } = useFetch('/api/stats/latest', { server: false });

const topOf = (counts: Record<string, number> | undefined) =>
    Object.entries(counts ?? {}).sort((a, b) => b[1] - a[1])[0]?.[0] ?? '—';

const statCards = computed(() => {
    const data = latest.value?.data;
    const isLoaded = latest.value !== undefined;

    return [
        {
            title: 'Docker',
            legend: 'of instances run in Docker',
            icon: IContainer,
            data: isLoaded ? formatShare(data?.docker ?? 0, data?.runtime ?? 0) : undefined,
        },
        {
            title: 'Platform',
            legend: 'most common operating system',
            icon: IMonitor,
            data: isLoaded ? topOf(data?.platforms) : undefined,
        },
        {
            title: 'CPU cores',
            legend: 'most common core count',
            icon: ICpu,
            data: isLoaded ? topOf(data?.cpuCores) : undefined,
        },
        {
            title: 'Memory',
            legend: 'most common memory size',
            icon: IMemoryStick,
            data: isLoaded ? topOf(data?.memory) : undefined,
        },
    ];
});

const charts = computed(() => {
    const data = latest.value?.data;

    return [
        {
            title: 'Operating systems',
            description: 'Platform reported by Node.js',
            items: toDonutItems(data?.platforms),
        },
        {
            title: 'Architectures',
            description: 'CPU architecture',
            items: toDonutItems(data?.arches),
        },
        {
            title: 'Node.js',
            description: 'Major Node.js version',
            items: toDonutItems(data?.node),
        },
        {
            title: 'CPU cores',
            description: 'Cores available to Fileship',
            items: toDonutItems(data?.cpuCores),
        },
        {
            title: 'Memory',
            description: 'Total memory, rounded to the nearest common size',
            items: toDonutItems(data?.memory),
        },
        {
            title: 'Disk',
            description: 'Size of the disk holding the data, rounded',
            items: toDonutItems(data?.disk),
        },
        {
            title: 'CPU models',
            description: 'The 10 most common CPU models',
            items: toDonutItems(data?.cpuModels),
        },
        {
            title: 'Docker',
            description: 'Instances running in a container',
            items: data
                ? [
                      { label: 'Docker', value: data.docker },
                      { label: 'Other', value: data.runtime - data.docker },
                  ].filter((item) => item.value)
                : [],
        },
    ];
});

definePageMeta({
    layout: 'panel',
});
</script>
