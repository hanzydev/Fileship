<template>
    <Head>
        <Title>Dashboard</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <div class="grid auto-rows-min gap-4 sm:grid-cols-2 xl:grid-cols-4">
            <StatCard v-for="card in statCards" :key="card.title" v-bind="card" />
        </div>
           <UiCard class="min-h-64">
            <UiCardContent></UiCardContent>
        </UiCard>
        <UiCard class="min-h-164 flex-1">
            <UiCardContent></UiCardContent>
        </UiCard>
    </div>
</template>

<script setup lang="ts">
import { IBox, IEye, IFiles, IUsers } from '#components';

const stats = useStats();

const isLoading = ref(true);

const statCards = computed(() => [
    {
        title: 'Files',
        legend: 'uploaded files',
        icon: IFiles,
        data: stats.value?.files?.count,
        growth: stats.value?.files?.growth,
    },
    {
        title: 'Views',
        legend: 'total file views',
        icon: IEye,
        data: stats.value?.views?.count,
        growth: stats.value?.views?.growth,
    },
    {
        title: 'Storage',
        legend: 'used storage',
        icon: IBox,
        data: stats.value?.storageUsed?.size,
        growth: stats.value?.storageUsed?.growth,
    },
    {
        title: 'Users',
        legend: 'total users',
        icon: IUsers,
        data: stats.value?.users?.count,
        growth: stats.value?.users?.growth,
    },
]);

onMounted(async () => {
    if (isLoading.value) {
        const statsData = await $fetch('/api/users/@me/stats');
        stats.value = statsData;
    }

    isLoading.value = false;
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'user-only',
});
</script>
