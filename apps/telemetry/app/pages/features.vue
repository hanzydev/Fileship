<template>
    <Head>
        <Title>Features</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader
            title="Features"
            :description="`Based on ${pluralize(latest?.data.full ?? 0, 'active instance')} running Fileship 6 or later.`"
        />

        <div class="grid auto-rows-min gap-4 sm:grid-cols-2 xl:grid-cols-4">
            <StatCard v-for="card in adoptionCards" :key="card.title" v-bind="card" />
        </div>

        <div class="grid auto-rows-min gap-4 sm:grid-cols-2 xl:grid-cols-4">
            <StatCard v-for="card in usageCards" :key="card.title" v-bind="card" />
        </div>

        <div class="grid auto-rows-min gap-4 sm:grid-cols-2 xl:grid-cols-4">
            <StatCard v-for="card in securityCards" :key="card.title" v-bind="card" />
        </div>

        <div class="flex flex-wrap gap-4">
            <StatsDonut
                v-for="chart in charts"
                :key="chart.title"
                class="min-w-0 grow basis-full lg:basis-[calc((100%-1rem)/2)] xl:basis-[calc((100%-2rem)/3)]"
                v-bind="chart"
                :is-loading="latest === undefined"
            />
        </div>
    </div>
</template>

<script setup lang="ts">
import {
    IBot,
    IDatabaseBackup,
    IEye,
    IEyeOff,
    IFingerprintPattern,
    IImageOff,
    IKeyRound,
    ILink,
    IMousePointerClick,
    IShieldCheck,
    IUpload,
    IUserPlus,
} from '#components';

const { data: latest } = useFetch('/api/stats/latest', { server: false });

const isLoaded = computed(() => latest.value !== undefined);

const share = (count?: number) =>
    isLoaded.value ? formatShare(count ?? 0, latest.value?.data.full ?? 0) : undefined;

const total = (count?: number) => (isLoaded.value ? (count ?? 0) : undefined);

const adoptionCards = computed(() => {
    const data = latest.value?.data;

    return [
        { title: 'AI', legend: 'of instances use AI', icon: IBot, data: share(data?.aiEnabled) },
        {
            title: 'Registration',
            legend: 'of instances allow sign-ups',
            icon: IUserPlus,
            data: share(data?.registration),
        },
        {
            title: 'Turnstile',
            legend: 'of instances protect logins with Turnstile',
            icon: IShieldCheck,
            data: share(data?.turnstile),
        },
        {
            title: 'EXIF removal',
            legend: 'of instances strip EXIF data from images',
            icon: IImageOff,
            data: share(data?.removeExifData),
        },
    ];
});

const securityCards = computed(() => {
    const data = latest.value?.data;

    return [
        {
            title: 'Authenticator app',
            legend: 'users with two-factor codes',
            icon: IKeyRound,
            data: total(data?.totp),
        },
        {
            title: 'Passkeys',
            legend: 'users with at least one passkey',
            icon: IFingerprintPattern,
            data: total(data?.passkeys),
        },
        {
            title: 'Upload tokens',
            legend: 'users with an upload token',
            icon: IUpload,
            data: total(data?.uploadTokens),
        },
        {
            title: 'Sensitive files',
            legend: `of ${formatNumber(data?.analyzedFiles ?? 0)} analyzed files`,
            icon: IEyeOff,
            data: total(data?.sensitiveFiles),
        },
    ];
});

const usageCards = computed(() => {
    const data = latest.value?.data;

    return [
        {
            title: 'Short links',
            legend: 'links on these instances',
            icon: ILink,
            data: total(data?.links),
        },
        {
            title: 'Link clicks',
            legend: 'clicks on short links',
            icon: IMousePointerClick,
            data: total(data?.clicks),
        },
        {
            title: 'Views',
            legend: 'file views in the last 30 days',
            icon: IEye,
            data: total(data?.views),
        },
        {
            title: 'Backups',
            legend: isLoaded.value
                ? `backups using ${formatBytes(data?.backupBytes ?? 0)}`
                : 'backups',
            icon: IDatabaseBackup,
            data: total(data?.backups),
        },
    ];
});

const charts = computed(() => {
    const data = latest.value?.data;
    const categories = Object.entries(data?.categories ?? {}).filter(([, value]) => value.total);

    return [
        {
            title: 'AI presets',
            description: 'Preset of the instances that use AI',
            valueLabel: 'Instances',
            items: toDonutItems(data?.presets),
        },
        {
            title: 'Active storage',
            description: 'Where new uploads are stored',
            valueLabel: 'Instances',
            items: toDonutItems(data?.activeStorages),
        },
        {
            title: 'Storage setups',
            description: 'Storages added besides the data directory',
            valueLabel: 'Instances',
            items: toDonutItems(data?.storageSetups),
        },
        {
            title: 'File types',
            description: 'Files per category',
            valueLabel: 'Files',
            items: categories.map(([label, value]) => ({ label, value: value.total })),
        },
        {
            title: 'Storage by type',
            description: 'Size of the files per category',
            valueLabel: 'Size',
            items: categories.map(([label, value]) => ({
                label,
                value: value.bytes,
                formatted: formatBytes(value.bytes),
            })),
        },
        {
            title: 'Trash retention',
            description: 'How long deleted items stay in the trash',
            valueLabel: 'Instances',
            items: toDonutItems(data?.trashRetention),
        },
    ];
});

definePageMeta({
    layout: 'panel',
});
</script>
