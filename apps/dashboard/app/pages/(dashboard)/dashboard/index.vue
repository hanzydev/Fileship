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

        <UiCard v-if="canUpload">
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
                    <UiSkeleton v-for="i in 6" :key="i" class="aspect-4/5 rounded-4xl" />
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
                        @click="sheetFileId = file.id"
                        @toggle="sheetFileId = file.id"
                        @view="sheetFileId = file.id"
                        @edit="openEdit(file)"
                        @move="openEdit(file)"
                    />
                </div>
            </UiCardContent>
        </UiCard>

        <FileSheet
            v-if="canUpload"
            v-model:file-id="sheetFileId"
            v-model:tab="sheetTab"
            :files="recentFiles"
        />
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';
import { upperFirst } from 'scule';

import { IBookmark, IBox, IEye, IFiles, IFolder, IUsers } from '#components';

const stats = useStats();
const files = useFiles();
const currentUser = useAuthUser();

const greeting = ref('Welcome back');
const today = ref('');
const sheetFileId = useQueryState('file', '');
const sheetTab = ref('details');

const canUpload = computed(() => canUploadFiles(currentUser.value));
const canNote = computed(() => canTakeNotes(currentUser.value));

const filesStatus = canUpload.value
    ? useLazyAsyncData('files', fetchFiles, { server: false }).status
    : ref('success');

const isFilesLoading = computed(() => filesStatus.value !== 'success' && !files.value.length);

const recentFiles = computed(() => files.value.slice(0, 6));

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
