<template>
    <Head>
        <Title>Statistics</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader
            title="Statistics"
            description="Instance-wide usage, refreshed every 30 minutes."
        />

        <div class="grid auto-rows-min gap-4 sm:grid-cols-2 xl:grid-cols-4">
            <StatCard v-for="card in statCards" :key="card.title" v-bind="card" />
        </div>

        <TrendChart
            title="Views"
            description="File views over the last 12 months"
            value-label="Views"
            :points="viewsPoints"
            :num-ticks="12"
            :is-loading="!stats && !statsFailed"
            :is-error="statsFailed"
        />

        <div class="grid gap-4 lg:grid-cols-3">
            <StatsDonut
                title="Top uploaders"
                description="Files uploaded per user"
                value-label="Files"
                :is-loading="!stats && !statsFailed"
                :items="topUploaders"
            />
            <StatsDonut
                title="Storage"
                description="Storage used per user"
                value-label="Size"
                :is-loading="!stats && !statsFailed"
                :items="storageByUser"
            />
            <StatsDonut
                title="File types"
                description="Most uploaded file types"
                value-label="Files"
                :is-loading="!stats && !statsFailed"
                :items="topTypes"
            />
        </div>
    </div>
</template>

<script setup lang="ts">
import { IBox, IEye, IFiles, IUsers } from '#components';

const { $toast } = useNuxtApp();
const stats = ref<any>(null);
const statsFailed = ref(false);

const findUser = (userId: string) => stats.value.users.all.find((u: any) => u.id === userId);

const viewsPoints = computed(() =>
    stats.value
        ? stats.value.views.byMonth.labels.map((month: string, index: number) => ({
              label: month,
              tick: month.slice(0, 3),
              value: stats.value.views.byMonth.data[index],
          }))
        : [],
);

const topUploaders = computed(() =>
    stats.value
        ? stats.value.topUploaders.map((u: any) => ({
              label: findUser(u.userId).username,
              avatar: findUser(u.userId).avatar,
              value: u.count,
          }))
        : [],
);

const storageByUser = computed(() =>
    stats.value
        ? stats.value.storageUsed.byUser.map((u: any) => ({
              label: findUser(u.userId).username,
              avatar: findUser(u.userId).avatar,
              value: +u.size,
              formatted: u.formattedSize,
          }))
        : [],
);

const topTypes = computed(() =>
    stats.value ? stats.value.topTypes.map((t: any) => ({ label: t.type, value: t.count })) : [],
);

const statCards = computed(() =>
    [
        {
            title: 'Users',
            legend: 'registered users',
            icon: IUsers,
            data: stats.value?.users.count,
            growth: stats.value?.users.growth,
        },
        {
            title: 'Files',
            legend: 'files on this instance',
            icon: IFiles,
            data: stats.value?.files.count,
            growth: stats.value?.files.growth,
        },
        {
            title: 'Storage',
            legend: +(stats.value?.storageUsed.trashed?.raw ?? 0)
                ? `total storage used, ${stats.value.storageUsed.trashed.formatted} in trash`
                : 'total storage used',
            icon: IBox,
            data: stats.value?.storageUsed.size,
            growth: stats.value?.storageUsed.growth,
        },
        {
            title: 'Views',
            legend: 'total file views',
            icon: IEye,
            data: stats.value?.views.count,
            growth: stats.value?.views.growth,
        },
    ].map((card) => (statsFailed.value ? { ...card, data: card.data ?? '—' } : card)),
);

onMounted(async () => {
    await $fetch('/api/stats')
        .then((data) => (stats.value = data))
        .catch((error) => {
            statsFailed.value = true;
            $toast.error(getErrorMessage(error));
        });
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'admin-only',
});
</script>
