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
                        :data="stats?.files?.count"
                        :growth="stats?.files?.growth"
                        :loading="isLoading"
                    />
                    <StatCard
                        title="Views"
                        description="total file views"
                        icon="heroicons-solid:eye"
                        :data="stats?.views?.count"
                        :growth="stats?.views?.growth"
                        :loading="isLoading"
                    />
                    <StatCard
                        title="Storage"
                        description="used storage"
                        icon="mdi:sd-storage"
                        :data="stats?.storageUsed?.size"
                        :growth="stats?.storageUsed?.growth"
                        :loading="isLoading"
                    />
                    <StatCard
                        title="Users"
                        description="total users"
                        icon="iconamoon:profile-fill"
                        :data="stats?.users?.count"
                        :growth="stats?.users?.growth"
                        :loading="isLoading"
                    />
                </div>
            </div>

            <div space-y-2>
                <h3>Views</h3>

                <Loading v-if="isLoading" />
                <div
                    v-else
                    min-h-365px
                    rounded-md
                    bg-fs-overlay-2
                    ring="1 fs-overlay-4"
                >
                    <ClientOnly>
                        <VueApexCharts
                            type="area"
                            height="350"
                            :options="{
                                labels: stats.views.byMonth.labels,
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
                                            Intl.NumberFormat().format(value),
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
                                    data: stats.views.byMonth.data,
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
                    ring="1 fs-overlay-4"
                >
                    <UiTable
                        :loading="isLoading"
                        :class="
                            stats?.topUploaders?.length ? 'xl:w-3/4' : 'wfull'
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
                                            }),
                                            h(
                                                'p',
                                                {
                                                    class: 'text-slate200',
                                                },
                                                titleCase(row.user.username),
                                            ),
                                        ],
                                    ),
                            },
                            {
                                key: 'count',
                                width: '20%',
                                resolve: ({ count }) =>
                                    Intl.NumberFormat().format(count),
                            },
                        ]"
                        :rows="stats?.topUploaders"
                        nothing-here-icon="heroicons-solid:document"
                        nothing-here-message="There is no data to display."
                    />
                    <div
                        v-if="stats?.topUploaders?.length && !isLoading"
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
                                    labels: stats.topUploaders.map((u: any) =>
                                        titleCase(u.user.username),
                                    ),
                                    fill: {
                                        colors: stats.topUploaders.map(
                                            (u: any) => colorHash(u.user.id),
                                        ),
                                    },
                                }"
                                :series="
                                    stats.topUploaders.map((u: any) => u.count)
                                "
                            />
                        </ClientOnly>
                    </div>
                </div>
            </div>

            <div space-y-2>
                <h3>Storage Used by User</h3>

                <div
                    flex="~ justify-between lt-xl:col"
                    rounded-md
                    bg-fs-overlay-2
                    ring="1 fs-overlay-4"
                >
                    <UiTable
                        :loading="isLoading"
                        :class="
                            stats?.storageUsed?.byUser?.length
                                ? 'xl:w-3/4'
                                : 'wfull'
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
                                            }),
                                            h(
                                                'p',
                                                {
                                                    class: 'text-slate200',
                                                },
                                                titleCase(row.user.username),
                                            ),
                                        ],
                                    ),
                            },
                            {
                                key: 'Used Storage',
                                width: '20%',
                                resolve: ({ formattedSize }) => formattedSize,
                            },
                        ]"
                        :rows="stats?.storageUsed?.byUser"
                        nothing-here-icon="heroicons-solid:document"
                        nothing-here-message="There is no data to display."
                    />
                    <div
                        v-if="stats?.storageUsed?.byUser?.length && !isLoading"
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
                                    labels: stats.storageUsed.byUser.map(
                                        (u: any) => titleCase(u.user.username),
                                    ),
                                    fill: {
                                        colors: stats.storageUsed.byUser.map(
                                            (u: any) => colorHash(u.user.id),
                                        ),
                                    },
                                    tooltip: {
                                        y: {
                                            formatter: filesize,
                                        },
                                    },
                                }"
                                :series="
                                    stats.storageUsed.byUser.map(
                                        (u: any) => +u.size,
                                    )
                                "
                            />
                        </ClientOnly>
                    </div>
                </div>
            </div>

            <div space-y-2>
                <h3>Top Types</h3>

                <div
                    flex="~ justify-between <xl:col"
                    rounded-md
                    bg-fs-overlay-2
                    ring="1 fs-overlay-4"
                >
                    <UiTable
                        :loading="isLoading"
                        :class="stats?.topTypes?.length ? 'xl:w3/4' : 'wfull'"
                        :columns="[
                            { key: 'type', width: '20%' },
                            {
                                key: 'count',
                                width: '20%',
                                resolve: ({ count }) =>
                                    Intl.NumberFormat().format(count),
                            },
                        ]"
                        :rows="stats?.topTypes"
                        nothing-here-icon="heroicons-solid:document"
                        nothing-here-message="There is no data to display."
                    />
                    <div
                        v-if="stats?.topTypes?.length && !isLoading"
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
                                    labels: stats.topTypes.map(
                                        (t: any) => t.type,
                                    ),
                                    fill: {
                                        colors: [
                                            ...new Set(stats.topTypes),
                                        ].map((t: any) => colorHash(t.type)),
                                    },
                                }"
                                :series="
                                    stats.topTypes.map((t: any) => t.count)
                                "
                            />
                        </ClientOnly>
                    </div>
                </div>
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
import { filesize } from 'filesize';
import { titleCase } from 'scule';
import VueApexCharts from 'vue3-apexcharts';

import { theme } from '@unocss/preset-mini';

import { UiAvatar } from '#components';

const stats = ref();
const isLoading = ref(true);

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
            formatter: (value: number) => Intl.NumberFormat().format(value),
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

onMounted(async () => {
    const data = await $fetch('/api/stats');

    stats.value = {
        ...data,
        topUploaders: data.topUploaders.map((u: any) => ({
            ...u,
            user: data.users.all.find((user: any) => user.id === u.userId),
        })),
        storageUsed: {
            ...data.storageUsed,
            byUser: data.storageUsed.byUser.map((u: any) => ({
                ...u,
                user: data.users.all.find((user: any) => user.id === u.userId),
            })),
        },
    };
    isLoading.value = false;
});

definePageMeta({
    layout: 'admin',
    middleware: 'admin-only',
});
</script>
