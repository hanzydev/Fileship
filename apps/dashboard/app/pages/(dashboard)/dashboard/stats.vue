<template>
    <Head>
        <Title>Statistics</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-6 p-4 pt-0">
        <PageHeader
            title="Statistics"
            description="Instance-wide usage, refreshed every 30 minutes."
        />

        <div class="grid auto-rows-min gap-4 sm:grid-cols-2 xl:grid-cols-4">
            <StatCard v-for="card in statCards" :key="card.title" v-bind="card" />
        </div>

        <UiCard>
            <UiCardHeader>
                <UiCardTitle>Views</UiCardTitle>
                <UiCardDescription>File views over the last 12 months</UiCardDescription>
            </UiCardHeader>
            <UiCardContent>
                <UiSkeleton v-if="!stats" class="h-64 w-full rounded-2xl" />
                <ViewsChart
                    v-else
                    :data="viewsData"
                    :num-ticks="12"
                    :tick-format="(label) => label.slice(0, 3)"
                    class="h-64"
                />
            </UiCardContent>
        </UiCard>

        <div class="grid gap-4 lg:grid-cols-3">
            <StatsDonut
                title="Top uploaders"
                description="Files uploaded per user"
                value-label="Files"
                :is-loading="!stats"
                :items="topUploaders"
            />
            <StatsDonut
                title="Storage"
                description="Storage used per user"
                value-label="Size"
                :is-loading="!stats"
                :items="storageByUser"
            />
            <StatsDonut
                title="File types"
                description="Most uploaded file types"
                value-label="Files"
                :is-loading="!stats"
                :items="topTypes"
            />
        </div>
    </div>
</template>

<script setup lang="ts">
import { IBox, IEye, IFiles, IUsers } from '#components';

const stats = ref<any>(null);

const findUser = (userId: string) => stats.value.users.all.find((u: any) => u.id === userId);

const viewsData = computed(() =>
    stats.value.views.byMonth.labels.map((label: string, index: number) => ({
        label,
        views: stats.value.views.byMonth.data[index],
    })),
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

const statCards = computed(() => [
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
        legend: 'total storage used',
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
]);

onMounted(async () => {
    stats.value = await $fetch('/api/stats');
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'admin-only',
});
</script>
