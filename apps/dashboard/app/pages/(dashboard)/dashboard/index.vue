<template>
    <Head>
        <Title>Dashboard</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-6 p-4 pt-0">
        <PageHeader
            :title="`${greeting}, ${upperFirst(currentUser!.username)}`"
            :description="today"
        >
            <UiButton v-if="canUpload" @click="uploadSheetOpen = true">
                <IUpload />
                Upload
            </UiButton>
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
                    class="grid grid-cols-2 gap-4 md:grid-cols-3 xl:grid-cols-4"
                >
                    <UiSkeleton v-for="i in 4" :key="i" class="aspect-4/5 rounded-4xl" />
                </div>
                <EmptyState
                    v-else-if="!recentFiles.length"
                    :icon="IFiles"
                    title="No files yet"
                    description="Drop files anywhere, paste an image or use the upload button."
                />
                <div v-else class="grid grid-cols-2 gap-4 md:grid-cols-3 xl:grid-cols-4">
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

        <div class="grid gap-4 lg:grid-cols-3">
            <UiCard v-if="canUpload">
                <UiCardHeader>
                    <UiCardTitle>Storage</UiCardTitle>
                    <UiCardDescription>
                        {{ filesize(totalSize) }} across {{ pluralize(files.length, 'file') }}
                    </UiCardDescription>
                </UiCardHeader>
                <UiCardContent class="flex flex-col gap-4">
                    <div class="flex h-3 w-full overflow-hidden rounded-full bg-muted">
                        <div
                            v-for="(item, index) in storageBreakdown"
                            :key="item.key"
                            class="h-full"
                            :style="{
                                width: `${(item.size / totalSize) * 100}%`,
                                background: `var(--chart-${(index % 5) + 1})`,
                            }"
                        />
                    </div>
                    <div class="flex flex-col gap-2">
                        <div
                            v-for="(item, index) in storageBreakdown"
                            :key="item.key"
                            class="flex items-center gap-2 text-sm"
                        >
                            <span
                                class="size-2.5 rounded-full"
                                :style="{ background: `var(--chart-${(index % 5) + 1})` }"
                            />
                            <span class="flex-1">{{ item.label }}</span>
                            <span class="text-muted-foreground">{{ item.count }}</span>
                            <span class="w-20 text-right">{{ filesize(item.size) }}</span>
                        </div>
                        <TextBlock v-if="!storageBreakdown.length" variant="muted">
                            Nothing stored yet.
                        </TextBlock>
                    </div>
                </UiCardContent>
            </UiCard>

            <UiCard v-if="canUpload">
                <UiCardHeader>
                    <UiCardTitle>Folders</UiCardTitle>
                    <UiCardAction>
                        <UiButton variant="ghost" size="sm" as-child>
                            <NuxtLink to="/dashboard/folders">
                                View all
                                <IArrowRight />
                            </NuxtLink>
                        </UiButton>
                    </UiCardAction>
                </UiCardHeader>
                <UiCardContent class="flex flex-col gap-1">
                    <NuxtLink
                        v-for="folder in recentFolders"
                        :key="folder.id"
                        :to="`/dashboard/folders/${folder.id}`"
                        class="flex items-center gap-3 rounded-xl px-2 py-1.5 hover:bg-muted"
                    >
                        <IInbox v-if="folder.isInbox" class="size-4 text-muted-foreground" />
                        <IFolder v-else class="size-4 text-muted-foreground" />
                        <span class="flex-1 truncate text-sm">{{ folder.name }}</span>
                        <span class="text-sm text-muted-foreground">{{ folder.files.length }}</span>
                    </NuxtLink>
                    <TextBlock v-if="!recentFolders.length" variant="muted">
                        No folders yet.
                    </TextBlock>
                </UiCardContent>
            </UiCard>

            <UiCard v-if="canNote">
                <UiCardHeader>
                    <UiCardTitle>Notes</UiCardTitle>
                    <UiCardAction>
                        <UiButton variant="ghost" size="sm" as-child>
                            <NuxtLink to="/dashboard/notes">
                                View all
                                <IArrowRight />
                            </NuxtLink>
                        </UiButton>
                    </UiCardAction>
                </UiCardHeader>
                <UiCardContent class="flex flex-col gap-1">
                    <NuxtLink
                        v-for="note in recentNotes"
                        :key="note.id"
                        :to="`/dashboard/notes/${note.id}`"
                        class="flex items-center gap-3 rounded-xl px-2 py-1.5 hover:bg-muted"
                    >
                        <IBookmark class="size-4 text-muted-foreground" />
                        <span class="flex-1 truncate text-sm">{{ note.title }}</span>
                        <span class="text-sm text-muted-foreground">
                            {{ dayjs(note.createdAt).fromNow() }}
                        </span>
                    </NuxtLink>
                    <TextBlock v-if="!recentNotes.length" variant="muted">No notes yet.</TextBlock>
                </UiCardContent>
            </UiCard>
        </div>

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
import { filesize } from 'filesize';
import { upperFirst } from 'scule';

import { IBookmark, IBox, IEye, IFiles, IFolder, IUsers } from '#components';

const stats = useStats();
const files = useFiles();
const notes = useNotes();
const folders = useFolders();
const currentUser = useAuthUser();
const uploadSheetOpen = useUploadSheet();

const greeting = ref('Welcome back');
const today = ref('');
const sheetFileId = useQueryState('file', '');
const sheetTab = ref('details');

const canUpload = computed(() => canUploadFiles(currentUser.value));
const canNote = computed(() => canTakeNotes(currentUser.value));

const filesStatus = canUpload.value
    ? useLazyAsyncData('files', fetchFiles, { server: false }).status
    : ref('success');

if (canUpload.value) useLazyAsyncData('folders', fetchFolders, { server: false });
if (canNote.value) useLazyAsyncData('notes', fetchNotes, { server: false });

const isFilesLoading = computed(() => filesStatus.value !== 'success' && !files.value.length);

const recentFiles = computed(() => files.value.slice(0, 8));
const recentFolders = computed(() => folders.value.slice(0, 5));
const recentNotes = computed(() => notes.value.slice(0, 5));

const totalSize = computed(() => files.value.reduce((total, f) => total + +f.size.raw, 0));

const storageBreakdown = computed(() => {
    const groups = new Map<FileCategory, { count: number; size: number }>();

    files.value.forEach((file) => {
        const category = getFileCategory(file);
        const group = groups.get(category) || { count: 0, size: 0 };

        group.count++;
        group.size += +file.size.raw;
        groups.set(category, group);
    });

    return [...groups.entries()]
        .map(([key, group]) => ({ key, label: FILE_CATEGORIES[key].label, ...group }))
        .sort((a, b) => b.size - a.size);
});

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
