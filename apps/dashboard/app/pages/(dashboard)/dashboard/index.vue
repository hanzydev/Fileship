<template>
    <Head>
        <Title>Dashboard</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-6 p-4 pt-0">
        <PageHeader
            :title="`${greeting}, ${upperFirst(currentUser!.username)}`"
            :description="today"
        >
            <UiButton v-if="canUpload" variant="outline" as-child>
                <NuxtLink to="/dashboard/folders?new=1">
                    <IFolderPlus />
                    New folder
                </NuxtLink>
            </UiButton>
            <UiButton v-if="canNote" variant="outline" as-child>
                <NuxtLink to="/dashboard/notes/new">
                    <ISquarePen />
                    New note
                </NuxtLink>
            </UiButton>
        </PageHeader>

        <div class="grid auto-rows-min gap-4 sm:grid-cols-2 xl:grid-cols-4">
            <StatCard v-for="card in statCards" :key="card.title" v-bind="card" />
        </div>

        <div v-if="canUpload" class="grid flex-1 gap-4 xl:grid-cols-3">
            <div class="flex min-w-0 flex-col gap-4 xl:col-span-2">
                <UiCard class="flex-1">
                    <UiCardHeader>
                        <UiCardTitle>Views</UiCardTitle>
                        <UiCardDescription>
                            Views of your files over the last 30 days
                        </UiCardDescription>
                    </UiCardHeader>
                    <UiCardContent class="relative min-h-32 flex-1">
                        <UiSkeleton
                            v-if="!stats?.views.daily"
                            class="absolute inset-x-6 inset-y-0"
                        />
                        <ViewsChart
                            v-else
                            :data="dailyViews"
                            :num-ticks="6"
                            class="absolute inset-x-6 inset-y-0 w-auto"
                        />
                    </UiCardContent>
                </UiCard>

                <UiCard>
                    <UiCardHeader>
                        <UiCardTitle>Recent files</UiCardTitle>
                        <UiCardDescription>Your latest uploads</UiCardDescription>
                        <UiCardAction>
                            <UiButton variant="ghost" size="sm" as-child>
                                <NuxtLink to="/dashboard/files">
                                    View all
                                    <IArrowRight />
                                </NuxtLink>
                            </UiButton>
                        </UiCardAction>
                    </UiCardHeader>
                    <UiCardContent>
                        <div
                            v-if="isFilesLoading"
                            class="grid grid-cols-2 gap-3 sm:grid-cols-3 lg:grid-cols-6"
                        >
                            <FileCardSkeleton v-for="i in 6" :key="i" />
                        </div>
                        <EmptyState
                            v-else-if="!recentFiles.length"
                            :icon="IFiles"
                            title="No files yet"
                            description="Drop files anywhere, paste an image or use the upload button."
                        />
                        <div v-else class="grid grid-cols-2 gap-3 sm:grid-cols-3 lg:grid-cols-6">
                            <FileCard
                                v-for="file in recentFiles"
                                :key="file.id"
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
                    </UiCardContent>
                </UiCard>
            </div>

            <StatsDonut
                title="Storage"
                description="Storage used by file type"
                value-label="Size"
                :is-loading="!stats?.storageByType"
                :items="storageByType"
                fill
            />
        </div>

        <template v-if="canUpload">
            <FileSheet v-model:file-id="sheetFileId" v-model:tab="sheetTab" :files="recentFiles" />
            <FileBulkBar v-model="selected" :items="recentFiles" :total="recentFiles.length" />
            <FileMoveDialog v-model="moveTargets" />
        </template>
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';
import { upperFirst } from 'scule';

import { IBookmark, IBox, IEye, IFiles, IFolder, IUsers } from '#components';

const stats = useStats();
const currentUser = useAuthUser();

const greeting = ref('Welcome back');
const today = ref('');
const sheetFileId = useQueryState('file', '');
const sheetTab = ref('details');
const selected = ref<string[]>([]);
const moveTargets = ref<Pick<FileData, 'id' | 'folderId'>[]>([]);

const canUpload = computed(() => canUploadFiles(currentUser.value));
const canNote = computed(() => canTakeNotes(currentUser.value));

const { items: recentFiles, isLoading: isFilesLoading } = usePaginatedList<FileData>(
    'files',
    '/api/files',
    {},
    { limit: 6, enabled: canUpload },
);

const dailyViews = computed(() =>
    stats.value!.views.daily!.labels.map((label, index) => ({
        label,
        views: stats.value!.views.daily!.data[index]!,
    })),
);

const storageByType = computed(() =>
    (stats.value?.storageByType ?? []).map((category) => ({
        label: FILE_CATEGORIES[category.type as FileCategory].label,
        value: +category.size,
        formatted: category.formatted,
    })),
);

const statCards = computed(() => [
    {
        title: 'Files',
        legend: 'uploaded files',
        icon: IFiles,
        data: stats.value?.files.count,
        growth: stats.value?.files.growth,
    },
    {
        title: 'Views',
        legend: 'total file views',
        icon: IEye,
        data: stats.value?.views.count,
        growth: stats.value?.views.growth,
    },
    {
        title: 'Storage',
        legend: 'used storage',
        icon: IBox,
        data: stats.value?.storageUsed.size,
        growth: stats.value?.storageUsed.growth,
    },
    isAdmin(currentUser.value)
        ? {
              title: 'Users',
              legend: 'total users',
              icon: IUsers,
              data: stats.value?.users.count,
              growth: stats.value?.users.growth,
          }
        : {
              title: canNote.value ? 'Notes' : 'Folders',
              legend: canNote.value ? 'saved notes' : 'created folders',
              icon: canNote.value ? IBookmark : IFolder,
              data: stats.value
                  ? currentUser.value!.stats[canNote.value ? 'notes' : 'folders']
                  : undefined,
              growth: stats.value ? 0 : undefined,
          },
]);

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
    const hour = new Date().getHours();

    today.value = dayjs().format('dddd, MMMM D');

    greeting.value =
        hour >= 5 && hour < 12
            ? 'Good morning'
            : hour >= 12 && hour < 17
              ? 'Good afternoon'
              : hour >= 17 && hour < 22
                ? 'Good evening'
                : 'Good night';

    stats.value = await $fetch('/api/users/@me/stats');
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'user-only',
});
</script>
