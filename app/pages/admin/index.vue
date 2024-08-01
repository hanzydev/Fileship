<template>
    <div>
        <Head>
            <Title>Admin Panel</Title>
        </Head>

        <div space-y-6>
            <h2>Admin Panel</h2>

            <div space-y-2>
                <h3>Statistics</h3>

                <div grid="~ gap6 md:cols-3 sm:cols-2 xl:cols-4">
                    <StatCard
                        title="Files"
                        description="uploaded files"
                        icon="heroicons-solid:document"
                        :data="data!.files.count"
                        :growth="data!.files.growth"
                    />
                    <StatCard
                        title="Views"
                        description="total file views"
                        icon="heroicons-solid:eye"
                        :data="data!.views.count"
                        :growth="data!.views.growth"
                    />
                    <StatCard
                        title="Storage"
                        description="used storage"
                        icon="mdi:sd-storage"
                        :data="data!.storageUsed.size"
                        :growth="data!.storageUsed.growth"
                    />
                    <StatCard
                        title="Users"
                        description="total users"
                        icon="iconamoon:profile-fill"
                        :data="data!.users.count"
                        :growth="data!.users.growth"
                    />
                </div>
            </div>

            <div space-y-2>
                <h3>Views</h3>

                <div min-h-365px rounded-md bg-fs-overlay-2>
                    <ClientOnly>
                        <VueApexCharts
                            type="area"
                            height="350"
                            :options="{
                                labels: data!.views.byMonth.labels,
                                colors: ['var(--fs-accent)'],
                                theme: {
                                    mode: 'dark',
                                },
                                dropShadow: {
                                    enabled: false,
                                },
                                chart: {
                                    background: 'transparent',
                                    fontFamily: 'Quicksand, sans-serif',
                                    foreColor: theme.colors.slate[200],
                                    toolbar: {
                                        show: false,
                                    },
                                    zoom: {
                                        enabled: false,
                                    },
                                },
                                stroke: {
                                    curve: 'smooth',
                                    colors: ['var(--fs-accent)'],
                                },
                                dataLabels: {
                                    enabled: false,
                                },
                                xaxis: {
                                    tooltip: {
                                        enabled: false,
                                    },
                                    crosshairs: {
                                        stroke: {
                                            color: 'var(--fs-accent)',
                                        },
                                    },
                                    axisBorder: {
                                        show: false,
                                    },
                                    axisTicks: {
                                        color: theme.colors.slate[200],
                                    },
                                },
                                tooltip: {
                                    y: {
                                        formatter: (value: number) =>
                                            value.toFixed(0),
                                    },
                                },
                                grid: {
                                    borderColor: 'var(--fs-background)',
                                },
                                fill: {
                                    colors: ['var(--fs-accent)'],
                                    gradient: {
                                        enabled: true,
                                        opacityFrom: 0.6,
                                        opacityTo: 0,
                                    },
                                },
                                markers: {
                                    colors: ['var(--fs-accent)'],
                                },
                            }"
                            :series="[
                                {
                                    name: 'Views',
                                    data: data!.views.byMonth.data,
                                },
                            ]"
                        />
                    </ClientOnly>
                </div>
            </div>

            <div space-y-2>
                <h3>Top Uploaders</h3>

                <div
                    flex="~ justify-between lt-xl:col"
                    rounded-md
                    bg-fs-overlay-2
                >
                    <UiTable
                        :class="
                            data!.topUploaders.length ? 'xl:w-3/4' : 'wfull'
                        "
                        :columns="[
                            {
                                key: 'user',
                                width: '20%',
                                render: (row) =>
                                    h(
                                        'div',
                                        {
                                            class: 'flex items-center gap2',
                                        },
                                        [
                                            h(UiAvatar, {
                                                size: 'xs',
                                                src: row.user.avatar,
                                                alt: row.user.username,
                                                class: '!ring-0',
                                            }),
                                            h(
                                                'p',
                                                {
                                                    class: 'text-slate200',
                                                },
                                                row.user.username,
                                            ),
                                        ],
                                    ),
                            },
                            { key: 'count', width: '20%' },
                        ]"
                        :rows="data!.topUploaders"
                        nothing-here-icon="heroicons-solid:document"
                        nothing-here-message="There is no data to display."
                    />
                    <div
                        v-if="data!.topUploaders.length"
                        hfull
                        w350px
                        py4
                        lt-xl:mxa
                        xl:mya
                        lt-sm:hidden
                    >
                        <ClientOnly>
                            <VueApexCharts
                                type="pie"
                                height="350"
                                :options="{
                                    ...basePieOptions,
                                    labels: data!.topUploaders.map(
                                        (u) => u.user.username,
                                    ),
                                    fill: {
                                        colors: data!.topUploaders.map((u) =>
                                            colorHash(u.user.id),
                                        ),
                                    },
                                }"
                                :series="data!.topUploaders.map((u) => u.count)"
                            />
                        </ClientOnly>
                    </div>
                </div>
            </div>

            <div space-y-2>
                <h3>Top Types</h3>

                <div
                    flex="~ justify-between lt-xl:col"
                    rounded-md
                    bg-fs-overlay-2
                >
                    <UiTable
                        :class="data!.topTypes.length ? 'xl:w3/4' : 'wfull'"
                        :columns="[
                            { key: 'type', width: '20%' },
                            { key: 'count', width: '20%' },
                        ]"
                        :rows="data!.topTypes"
                        nothing-here-icon="heroicons-solid:document"
                        nothing-here-message="There is no data to display."
                    />
                    <div
                        v-if="data!.topTypes.length"
                        hfull
                        w350px
                        py4
                        lt-xl:mxa
                        xl:mya
                        lt-sm:hidden
                    >
                        <ClientOnly>
                            <VueApexCharts
                                type="pie"
                                height="350"
                                :options="{
                                    ...basePieOptions,
                                    labels: data!.topTypes.map((t) => t.type),
                                    fill: {
                                        colors: [
                                            ...new Set(data!.topTypes),
                                        ].map((t) => colorHash(t.type)),
                                    },
                                }"
                                :series="data!.topTypes.map((t) => t.count)"
                            />
                        </ClientOnly>
                    </div>
                </div>
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
import VueApexCharts from 'vue3-apexcharts';

import { theme } from '@unocss/preset-mini';

import { UiAvatar } from '#components';

const { data } = await useFetch('/api/stats');

const basePieOptions = {
    colors: ['var(--fs-overlay-1)'],
    theme: {
        mode: 'dark',
    },
    dropShadow: {
        enabled: false,
    },
    chart: {
        background: 'transparent',
        fontFamily: 'Quicksand, sans-serif',
        foreColor: theme.colors.slate[200],
        toolbar: {
            show: false,
        },
    },
    dataLabels: {
        enabled: false,
    },
    legend: {
        show: false,
    },
    tooltip: {
        y: {
            formatter: (value: number) => value.toFixed(0),
        },
    },
    grid: {
        borderColor: 'var(--fs-background)',
    },
    plotOptions: {
        pie: {
            expandOnClick: false,
        },
    },
    states: {
        active: {
            filter: {
                type: 'none',
            },
        },
        hover: {
            filter: {
                type: 'none',
            },
        },
    },
};

definePageMeta({
    layout: 'admin',
    middleware: 'admin-only',
});
</script>
