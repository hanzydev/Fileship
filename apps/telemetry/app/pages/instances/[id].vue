<template>
    <Head>
        <Title>{{ shortId }}</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader
            :title="shortId"
            :description="
                instance
                    ? `First seen ${dayjs(instance.firstSeenAt).fromNow()}, last report ${dayjs(instance.lastSeenAt).fromNow()}`
                    : 'Loading...'
            "
        >
            <UiBadge v-if="instance?.version" variant="secondary">v{{ instance.version }}</UiBadge>
        </PageHeader>

        <UiAlert v-if="instance">
            <IInfo />
            <UiAlertTitle>Kept while it reports</UiAlertTitle>
            <UiAlertDescription>
                This instance and its history are deleted after {{ instance.retentionDays }} days
                without a report, on
                {{ dayjs(instance.lastSeenAt).add(instance.retentionDays, 'day').format('LL') }}.
            </UiAlertDescription>
        </UiAlert>

        <div class="grid auto-rows-min gap-4 sm:grid-cols-2 xl:grid-cols-4">
            <StatCard v-for="card in statCards" :key="card.title" v-bind="card" />
        </div>

        <div class="grid gap-4 lg:grid-cols-2">
            <TrendChart
                title="Users"
                description="Users over the last year"
                value-label="Users"
                color="var(--chart-3)"
                :points="points('users')"
                :is-loading="!instance"
            />
            <TrendChart
                title="Files"
                description="Files over the last year, trash excluded"
                value-label="Files"
                color="var(--chart-4)"
                :points="points('files')"
                :is-loading="!instance"
            />
        </div>

        <div class="grid gap-4 lg:grid-cols-2">
            <UiCard v-for="section in sections" :key="section.title">
                <UiCardHeader>
                    <UiCardTitle>{{ section.title }}</UiCardTitle>
                    <UiCardDescription>{{ section.description }}</UiCardDescription>
                </UiCardHeader>
                <UiCardContent>
                    <template v-if="!instance">
                        <TextBlock v-for="i in 6" :key="i" variant="muted" class="w-full py-1">
                            <UiSkeleton class="h-lh w-full rounded-full" />
                        </TextBlock>
                    </template>
                    <TextBlock v-else-if="!section.rows.length" variant="muted" class="text-center">
                        Reported by Fileship 6 and later.
                    </TextBlock>
                    <dl v-else class="grid grid-cols-[auto_1fr] gap-x-6 gap-y-2 text-sm">
                        <template v-for="row in section.rows" :key="row.label">
                            <dt class="text-muted-foreground">{{ row.label }}</dt>
                            <dd class="truncate text-right" :title="row.value">{{ row.value }}</dd>
                        </template>
                    </dl>
                </UiCardContent>
            </UiCard>
        </div>
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

import { IBox, IFiles, ILink, IUsers } from '#components';

const route = useRoute();
const labels = useBreadcrumbLabels();

const { data: instance, error } = useFetch(`/api/instances/${route.params.id}`, {
    server: false,
});

const shortId = computed(() => (route.params.id as string).slice(0, 8));

const yesNo = (value?: boolean) => (value === undefined ? null : value ? 'Yes' : 'No');

const statCards = computed(() => {
    const usage = instance.value?.report.usage;
    const isLoaded = !!instance.value;

    return [
        {
            title: 'Users',
            legend: pluralize(usage?.users?.admins ?? 0, 'admin'),
            icon: IUsers,
            data: isLoaded ? (usage?.users?.total ?? 0) : undefined,
        },
        {
            title: 'Files',
            legend: `${formatNumber(usage?.files?.trashed ?? 0)} in the trash`,
            icon: IFiles,
            data: isLoaded ? (usage?.files?.total ?? 0) : undefined,
        },
        {
            title: 'Storage',
            legend: 'used by files',
            icon: IBox,
            data: isLoaded ? formatBytes(usage?.files?.bytes ?? 0) : undefined,
        },
        {
            title: 'Short links',
            legend: pluralize(usage?.links?.clicks ?? 0, 'click'),
            icon: ILink,
            data: isLoaded ? (usage?.links?.total ?? 0) : undefined,
        },
    ];
});

const points = (key: 'users' | 'files') =>
    (instance.value?.history ?? []).map((day) => ({ day: day.day, value: day[key] }));

const sections = computed(() => {
    const { runtime, usage, features } = instance.value?.report ?? {};

    const rows = (entries: [string, string | number | null | undefined][]) =>
        entries
            .filter(([, value]) => value !== null && value !== undefined && value !== '')
            .map(([label, value]) => ({
                label,
                value: typeof value === 'number' ? formatNumber(value) : value!,
            }));

    return [
        {
            title: 'System',
            description: 'Where this instance runs',
            rows: rows([
                ['Platform', runtime?.platform && `${runtime.platform} ${runtime.release ?? ''}`],
                ['Architecture', runtime?.arch],
                ['Node.js', runtime?.node],
                ['CPU', runtime?.cpuModel && `${runtime.cpuModel} (${runtime.cpuCount} cores)`],
                ['Memory', runtime?.memoryTotal ? formatBytes(runtime.memoryTotal) : null],
                [
                    'Disk',
                    runtime?.diskTotal
                        ? `${formatBytes(runtime.diskFree ?? 0)} free of ${formatBytes(runtime.diskTotal)}`
                        : null,
                ],
                ['Docker', yesNo(runtime?.docker)],
                [
                    'Uptime',
                    runtime?.uptime ? dayjs.duration(runtime.uptime, 'seconds').humanize() : null,
                ],
            ]),
        },
        {
            title: 'Features',
            description: 'Settings and usage reported by Fileship',
            rows: features
                ? rows([
                      ['AI', features.ai?.enabled ? `On, ${features.ai.preset} preset` : 'Off'],
                      ['Analyzed files', features.ai?.analyzedFiles],
                      ['Sensitive files', features.ai?.sensitiveFiles],
                      ['Registration', features.registration ? 'Open' : 'Closed'],
                      ['Turnstile', yesNo(features.turnstile)],
                      ['EXIF removal', yesNo(features.removeExifData)],
                      [
                          'Trash',
                          features.trashRetentionDays
                              ? `${features.trashRetentionDays} days`
                              : 'Off',
                      ],
                      ['Active storage', usage?.storages?.active],
                      ['Backups', usage?.backups?.total],
                      ['Views (30 days)', usage?.views?.last30Days],
                  ])
                : [],
        },
    ];
});

watch(error, (value) => {
    if (value) showError(value);
});

watch(
    instance,
    () => {
        labels.value[route.params.id as string] = shortId.value;
    },
    { immediate: true },
);

definePageMeta({
    layout: 'panel',
});
</script>
