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
            <UiCard class="min-w-0 xl:col-span-2">
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
                <UiCardContent class="@container">
                    <EmptyState
                        v-if="!isFilesLoading && !recentFiles.length"
                        :icon="IFiles"
                        title="No files yet"
                        description="Drop files anywhere, paste an image or use the upload button."
                    >
                        <UiButton @click="uploadSheetOpen = true">
                            <IUpload />
                            Upload files
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
                    title="File types"
                    description="Files by type"
                    value-label="Files"
                    :is-loading="!stats"
                    :items="filesByType"
                    compact
                />
                <StatsDonut
                    title="Storage"
                    description="Storage used by file type"
                    value-label="Size"
                    :is-loading="!stats"
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
import dayjs from 'dayjs';
import { upperFirst } from 'scule';

import { IBookmark, IBox, IEye, IFiles, IFolder, IUsers } from '#components';

const stats = useStats();
const currentUser = useAuthUser();
const uploadSheetOpen = useUploadSheet();

const greeting = ref('Welcome back');
const today = ref('');
const sheetFileId = useQueryState('file', '');
const sheetTab = ref('details');
const selected = ref<string[]>([]);
const moveTargets = ref<Pick<FileData, 'id' | 'folderId'>[]>([]);

const canUpload = computed(() => canUploadFiles(currentUser.value));
const canNote = computed(() => canTakeNotes(currentUser.value));

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
        label: FILE_CATEGORIES[category.type as FileCategory].label,
        value: category.count,
    })),
);

const storageByType = computed(() =>
    (stats.value?.byType ?? []).map((category) => ({
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
              data: stats.value?.[canNote.value ? 'notes' : 'folders'].count,
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
