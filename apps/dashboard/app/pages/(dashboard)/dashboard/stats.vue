<template>
    <Head>
        <Title>{{ $t('stats.title') }}</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader :title="$t('stats.title')" :description="$t('stats.description')" />

        <div class="grid auto-rows-min gap-4 sm:grid-cols-2 xl:grid-cols-4">
            <StatCard v-for="card in statCards" :key="card.title" v-bind="card" />
        </div>

        <TrendChart
            :title="$t('stats.views')"
            :description="$t('stats.viewsDescription')"
            :value-label="$t('stats.views')"
            :points="viewsPoints"
            :num-ticks="12"
            :is-loading="!stats && !statsFailed"
            :is-error="statsFailed"
        />

        <div class="grid gap-4 lg:grid-cols-3">
            <StatsDonut
                :title="$t('stats.topUploaders')"
                :description="$t('stats.topUploadersDescription')"
                :value-label="$t('stats.files')"
                :is-loading="!stats && !statsFailed"
                :items="topUploaders"
            />
            <StatsDonut
                :title="$t('stats.storage')"
                :description="$t('stats.storageDescription')"
                :value-label="$t('stats.size')"
                :is-loading="!stats && !statsFailed"
                :items="storageByUser"
            />
            <StatsDonut
                :title="$t('stats.fileTypes')"
                :description="$t('stats.fileTypesDescription')"
                :value-label="$t('stats.files')"
                :is-loading="!stats && !statsFailed"
                :items="topTypes"
            />
        </div>
    </div>
</template>

<script setup lang="ts">
import { IBox, IEye, IFiles, IUsers } from '#components';

const { $toast } = useNuxtApp();
const { t, locale } = useI18n();
const date = useDate();
const stats = ref<any>(null);
const statsFailed = ref(false);

const findUser = (userId: string) => stats.value.users.all.find((u: any) => u.id === userId);

const viewsPoints = computed(() =>
    stats.value
        ? stats.value.views.byMonth.data.map((value: number, index: number) => {
              const month = date()
                  .subtract(11 - index, 'month')
                  .toDate();

              return {
                  label: new Intl.DateTimeFormat(locale.value, {
                      month: 'long',
                      year: 'numeric',
                  }).format(month),
                  tick: new Intl.DateTimeFormat(locale.value, { month: 'short' }).format(month),
                  value,
              };
          })
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
            title: t('stats.cards.users'),
            legend: t('stats.cards.usersLegend'),
            icon: IUsers,
            data: stats.value?.users.count,
            growth: stats.value?.users.growth,
        },
        {
            title: t('stats.cards.files'),
            legend: t('stats.cards.filesLegend'),
            icon: IFiles,
            data: stats.value?.files.count,
            growth: stats.value?.files.growth,
        },
        {
            title: t('stats.cards.storage'),
            legend: +(stats.value?.storageUsed.trashed?.raw ?? 0)
                ? t('stats.cards.storageLegendTrashed', {
                      size: stats.value.storageUsed.trashed.formatted,
                  })
                : t('stats.cards.storageLegend'),
            icon: IBox,
            data: stats.value?.storageUsed.size,
            growth: stats.value?.storageUsed.growth,
        },
        {
            title: t('stats.cards.views'),
            legend: t('stats.cards.viewsLegend'),
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
