<template>
    <Head>
        <Title>{{ $t('home.title') }}</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader :title="greeting" :description="today">
            <UiButton v-if="canUpload" variant="outline" as-child>
                <NuxtLink to="/dashboard/folders?new=1">
                    <IFolderPlus />
                    {{ $t('home.newFolder') }}
                </NuxtLink>
            </UiButton>
            <UiButton v-if="canNote" variant="outline" as-child>
                <NuxtLink to="/dashboard/notes/new">
                    <ISquarePen />
                    {{ $t('home.newNote') }}
                </NuxtLink>
            </UiButton>
        </PageHeader>

        <div class="grid auto-rows-min gap-4 sm:grid-cols-2 xl:grid-cols-4">
            <StatCard v-for="card in statCards" :key="card.title" v-bind="card" />
        </div>

        <div v-if="canUpload" class="grid flex-1 gap-4 xl:grid-cols-3">
            <UiCard class="min-w-0 xl:col-span-2">
                <UiCardHeader>
                    <UiCardTitle>{{ $t('home.recentFiles') }}</UiCardTitle>
                    <UiCardDescription>{{ $t('home.recentFilesDescription') }}</UiCardDescription>
                    <UiCardAction>
                        <UiButton variant="ghost" size="sm" as-child>
                            <NuxtLink to="/dashboard/files">
                                {{ $t('home.viewAll') }}
                                <IArrowRight />
                            </NuxtLink>
                        </UiButton>
                    </UiCardAction>
                </UiCardHeader>
                <UiCardContent class="@container">
                    <EmptyState
                        v-if="!isFilesLoading && !recentFiles.length"
                        :icon="IFiles"
                        :title="$t('home.noFiles')"
                        :description="$t('home.noFilesDescription')"
                    >
                        <UiButton @click="uploadSheetOpen = true">
                            <IUpload />
                            {{ $t('home.uploadFiles') }}
                        </UiButton>
                    </EmptyState>
                    <div
                        v-else
                        class="grid grid-cols-2 gap-3 @lg:grid-cols-3 @2xl:grid-cols-4 @4xl:grid-cols-5 @6xl:grid-cols-6"
                    >
                        <template v-if="isFilesLoading">
                            <div v-for="i in 12" :key="i" :class="recentItemClass(i - 1)">
                                <FileCardSkeleton />
                            </div>
                        </template>
                        <div
                            v-for="(file, index) in gridFiles"
                            v-else
                            :key="file.id"
                            :class="recentItemClass(index)"
                        >
                            <FileCard
                                :file="file"
                                :selected="selected.includes(file.id)"
                                :selection-mode="!!selected.length"
                                @click="handleClick(file)"
                                @toggle="handleToggle(file)"
                                @view="sheetFileId = file.id"
                                @edit="openEdit(file)"
                                @move="moveTargets = [file]"
                            />
                        </div>
                    </div>
                </UiCardContent>
            </UiCard>

            <div class="grid gap-4 sm:grid-cols-2 xl:grid-cols-1">
                <StatsDonut
                    :title="$t('home.fileTypes')"
                    :description="$t('home.fileTypesDescription')"
                    :value-label="$t('home.files')"
                    :is-loading="!stats && !statsFailed"
                    :items="filesByType"
                    compact
                />
                <StatsDonut
                    :title="$t('home.storage')"
                    :description="$t('home.storageDescription')"
                    :value-label="$t('home.size')"
                    :is-loading="!stats && !statsFailed"
                    :items="storageByType"
                    compact
                />
            </div>
        </div>

        <template v-if="canUpload">
            <FileViewer
                v-model:file-id="sheetFileId"
                v-model:tab="sheetTab"
                :files="recentFiles"
                :total="total"
                :has-more="hasMore"
                :load-more="loadMore"
            />
            <FileBulkBar v-model="selected" :items="gridFiles" :total="gridFiles.length" />
            <FileMoveDialog v-model="moveTargets" />
        </template>
    </div>
</template>

<script setup lang="ts">
import { upperFirst } from 'scule';

import { IBookmark, IBox, IEye, IFiles, IFolder, ILink, IUsers } from '#components';

import type { FileData } from '~/types/files';

const { $toast } = useNuxtApp();
const { t, locale } = useI18n();
const stats = useStats();
const storageUsage = useStorageUsage();
const statsFailed = ref(false);
const currentUser = useAuthUser();
const uploadSheetOpen = useUploadSheet();

const greetingKey = ref('welcomeBack');
const now = ref<Date | null>(null);

const today = computed(() =>
    now.value
        ? new Intl.DateTimeFormat(locale.value, {
              weekday: 'long',
              month: 'long',
              day: 'numeric',
          }).format(now.value)
        : '',
);
const sheetFileId = useQueryState('file', '');
const sheetTab = ref('details');
const selected = ref<string[]>([]);
const moveTargets = ref<Pick<FileData, 'id' | 'folderId'>[]>([]);

const canUpload = computed(() => canUploadFiles(currentUser.value));
const canNote = computed(() => canTakeNotes(currentUser.value));
const canShorten = computed(() => canShortenLinks(currentUser.value));

const greeting = computed(() =>
    t(`home.greeting.${greetingKey.value}`, { name: upperFirst(currentUser.value!.username) }),
);

const {
    items: recentFiles,
    total,
    hasMore,
    loadMore,
    isLoading: isFilesLoading,
} = usePaginatedList<FileData>('files', '/api/files', {}, { limit: 12, enabled: canUpload });

const gridFiles = computed(() => recentFiles.value.slice(0, 12));

const recentItemClass = (index: number) =>
    cn(
        index >= 4 && 'hidden',
        index >= 4 && index < 6 && '@lg:block',
        index >= 6 && index < 8 && '@2xl:block',
        index >= 8 && index < 10 && '@4xl:block',
        index >= 10 && '@6xl:block',
    );

const filesByType = computed(() =>
    (stats.value?.byType ?? []).map((category) => ({
        label: t(FILE_CATEGORIES[category.type as FileCategory].label),
        value: category.count,
    })),
);

const storageByType = computed(() =>
    (stats.value?.byType ?? []).map((category) => ({
        label: t(FILE_CATEGORIES[category.type as FileCategory].label),
        value: +category.size,
        formatted: category.formatted,
    })),
);

const storageRatio = computed(() =>
    storageUsage.value?.limit
        ? Number(storageUsage.value.used) / Number(storageUsage.value.limit)
        : null,
);

const storageLegend = computed(() => {
    const trashed = +(stats.value?.storageUsed.trashed?.raw ?? 0)
        ? stats.value!.storageUsed.trashed.formatted
        : null;

    if (storageRatio.value === null) {
        return trashed
            ? t('home.cards.storageLegendTrashed', { size: trashed })
            : t('home.cards.storageLegend');
    }

    const quota = {
        percent: new Intl.NumberFormat(locale.value, {
            style: 'percent',
            maximumFractionDigits: 0,
        }).format(storageRatio.value),
        limit: formatStorageSize(Number(storageUsage.value!.limit), locale.value),
    };

    return trashed
        ? t('home.cards.storageQuotaTrashed', { ...quota, size: trashed })
        : t('home.cards.storageQuota', quota);
});

const statCards = computed(() =>
    [
        {
            title: t('home.cards.files'),
            legend: t('home.cards.filesLegend'),
            icon: IFiles,
            data: stats.value?.files.count,
            growth: stats.value?.files.growth,
        },
        {
            title: t('home.cards.views'),
            legend: t('home.cards.viewsLegend'),
            icon: IEye,
            data: stats.value?.views.count,
            growth: stats.value?.views.growth,
        },
        {
            title: t('home.cards.storage'),
            legend: storageLegend.value,
            icon: IBox,
            data: stats.value?.storageUsed.size,
            growth: stats.value?.storageUsed.growth,
            progress: storageRatio.value === null ? undefined : storageRatio.value * 100,
        },
        isAdmin(currentUser.value)
            ? {
                  title: t('home.cards.users'),
                  legend: t('home.cards.usersLegend'),
                  icon: IUsers,
                  data: stats.value?.users.count,
                  growth: stats.value?.users.growth,
              }
            : canNote.value
              ? {
                    title: t('home.cards.notes'),
                    legend: t('home.cards.notesLegend'),
                    icon: IBookmark,
                    data: stats.value?.notes.count,
                }
              : canShorten.value && !canUpload.value
                ? {
                      title: t('home.cards.links'),
                      legend: t('home.cards.linksLegend'),
                      icon: ILink,
                      data: stats.value?.links.count,
                  }
                : {
                      title: t('home.cards.folders'),
                      legend: t('home.cards.foldersLegend'),
                      icon: IFolder,
                      data: stats.value?.folders.count,
                  },
    ].map((card) => (statsFailed.value ? { ...card, data: card.data ?? '—' } : card)),
);

const handleToggle = (file: FileData) => {
    selected.value = selected.value.includes(file.id)
        ? selected.value.filter((id) => id !== file.id)
        : [...selected.value, file.id];
};

const handleClick = (file: FileData) => {
    if (selected.value.length) handleToggle(file);
    else sheetFileId.value = file.id;
};

const openEdit = (file: FileData) => {
    sheetTab.value = 'edit';
    sheetFileId.value = file.id;
};

onMounted(async () => {
    now.value = new Date();
    const hour = now.value.getHours();

    greetingKey.value =
        hour >= 5 && hour < 12
            ? 'morning'
            : hour >= 12 && hour < 17
              ? 'afternoon'
              : hour >= 17 && hour < 22
                ? 'evening'
                : 'night';

    await $fetch('/api/users/@me/stats')
        .then((data) => (stats.value = data))
        .catch((error) => {
            statsFailed.value = true;
            $toast.error(getErrorMessage(error));
        });
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'user-only',
});
</script>
