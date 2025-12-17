<template>
    <Head>
        <Title>Dashboard</Title>
    </Head>

    <ModalsViewFile v-if="viewModal.fileId" v-model="viewModal.open" :file-id="viewModal.fileId" />

    <ModalsEditFile
        v-if="editModal.file"
        :key="editModal.file!.id"
        v-model="editModal.open"
        :data="editModal.file!"
    />

    <DashboardContent>
        <template #header>
            <h2 lt-md="text-2xl!">
                {{ greetingMessage }}, {{ upperFirst(currentUser!.username) }} 👋
            </h2>
        </template>
        <div space-y-2>
            <h3>Your Statistics</h3>

            <div grid="~ gap6 lg:cols-3 md:cols-2 xl:cols-4">
                <StatCard
                    title="Files"
                    description="uploaded files"
                    icon="solar:documents-bold"
                    :data="stats?.files?.count"
                    :growth="stats?.files?.growth"
                    :loading="isLoading"
                />
                <StatCard
                    title="Views"
                    description="total file views"
                    icon="solar:eye-bold"
                    :data="stats?.views?.count"
                    :growth="stats?.views?.growth"
                    :loading="isLoading"
                />
                <StatCard
                    title="Storage"
                    description="used storage"
                    icon="solar:diskette-bold"
                    :data="stats?.storageUsed?.size"
                    :growth="stats?.storageUsed?.growth"
                    :loading="isLoading"
                />
                <StatCard
                    title="Users"
                    description="total users"
                    icon="solar:users-group-rounded-bold"
                    :data="stats?.users?.count"
                    :growth="stats?.users?.growth"
                    :loading="isLoading"
                />
            </div>
        </div>

        <div pt6 space-y-2>
            <h3>Recent Files</h3>
            <div
                v-show="filteredFiles.length || isLoading"
                grid="~ gap6 lg:cols-3 md:cols-2 xl:cols-4"
            >
                <template v-if="isLoading">
                    <UiSkeletonCard
                        v-for="i in 4"
                        :key="i"
                        flex="~ col items-center justify-center gap2"
                        h208px
                    >
                        <Icon
                            v-if="randomNumber(0, 1) === 0"
                            name="solar:play-bold"
                            size="64"
                            animate-pulse
                            op75
                        />
                        <Icon v-else name="solar:gallery-bold" size="64" animate-pulse op75 />
                    </UiSkeletonCard>
                </template>
                <TransitionGroup
                    v-else
                    :css="false"
                    @enter="
                        (el, done) => {
                            (el as HTMLDivElement).style.opacity = '1';
                            done();
                        }
                    "
                >
                    <div
                        v-for="file in filteredFiles.slice(0, 4)"
                        :key="file.id"
                        op0
                        class="fileCard"
                    >
                        <FileCard
                            :data="file"
                            @view-file="
                                (file) => {
                                    viewModal.fileId = file.id;
                                    nextTick(() => (viewModal.open = true));
                                }
                            "
                        />
                    </div>
                </TransitionGroup>
            </div>
            <NothingHere
                v-if="!filteredFiles.length && !isLoading"
                message="There are no files to display."
                icon="solar:documents-bold"
            />
        </div>

        <div pt6 space-y-2>
            <h3>Files</h3>
            <div space-y-4>
                <UiTable
                    ring="1 fs-overlay-4"
                    :loading="isLoading"
                    :columns="[
                        {
                            key: 'fileName',
                            width: '25%',
                        },
                        {
                            key: 'mimeType',
                            width: '20%',
                        },
                        {
                            key: 'size',
                            width: '10%',
                            resolve: ({ size }) => size.formatted,
                        },
                        {
                            key: 'createdAt',
                            width: '15%',
                            resolve: ({ createdAt }) => dayjs(createdAt).fromNow(),
                        },
                        {
                            key: 'Quick Actions',
                            width: '20%',
                            render: (row) => {
                                const isImage = row.mimeType.startsWith('image/');
                                const isVideo = row.mimeType.startsWith('video/');
                                const isAudio = row.mimeType.startsWith('audio/');

                                const canBeViewed = isImage || isVideo || isAudio;

                                return h('div', { class: 'flex items-center gap4' }, [
                                    h(UiButton, {
                                        variant: 'outline',
                                        alignment: 'center',
                                        class: 'h8 w8 !p0 text-fs-muted-2 hover:text-white',
                                        icon: canBeViewed
                                            ? 'solar:eye-bold'
                                            : 'lucide:external-link',
                                        iconSize: '20',
                                        'aria-label': canBeViewed
                                            ? 'View file'
                                            : 'Open file in new tab',
                                        ...(canBeViewed
                                            ? {
                                                  onClick: () => {
                                                      viewModal.fileId = row.id;
                                                      nextTick(() => (viewModal.open = true));
                                                  },
                                              }
                                            : {
                                                  href: row.embedUrl,
                                                  target: '_blank',
                                              }),
                                    }),
                                    h(UiButton, {
                                        variant: 'outline',
                                        alignment: 'center',
                                        class: [
                                            'h8 w8 !p0 hover:text-white',
                                            copiedFiles.has(row.fileName)
                                                ? 'text-green500!'
                                                : 'text-fs-muted-2',
                                        ],
                                        icon: copiedFiles.has(row.fileName)
                                            ? 'solar:clipboard-check-bold'
                                            : 'solar:clipboard-bold',
                                        iconSize: '20',
                                        'aria-label': 'Copy link to clipboard',
                                        onClick: () => handleCopy(row),
                                    }),
                                    h(UiButton, {
                                        variant: 'outline',
                                        alignment: 'center',
                                        class: 'h8 w8 !p0 text-fs-muted-2 hover:text-white',
                                        href: `${row.directUrl}?download`,
                                        target: '_blank',
                                        icon: 'solar:download-minimalistic-bold',
                                        iconSize: '20',
                                        'aria-label': 'Download file',
                                    }),
                                    h(UiButton, {
                                        variant: 'outline',
                                        alignment: 'center',
                                        class: 'h8 w8 !p0 text-fs-muted-2 hover:text-white',
                                        icon: 'solar:pen-2-bold',
                                        iconSize: '20',
                                        'aria-label': 'Edit file',
                                        onClick: () => {
                                            editModal.file = row;
                                            nextTick(() => (editModal.open = true));
                                        },
                                    }),
                                    h(UiButton, {
                                        variant: 'outline',
                                        alignment: 'center',
                                        class: 'h8 w8 !p0 ring-red-500 text-fs-muted-2 hover:text-white hover:!bg-red-500',
                                        icon: 'solar:trash-bin-minimalistic-bold',
                                        iconSize: '20',
                                        disabled: willBeDeleted.has(row.id),
                                        loading: willBeDeleted.has(row.id),
                                        'aria-label': 'Delete file',
                                        onClick: () => handleDelete(row.id),
                                    }),
                                ]);
                            },
                        },
                    ]"
                    :rows="calculatedFiles"
                    nothing-here-message="There are no files to display."
                    nothing-here-icon="solar:documents-bold"
                />
                <UiPagination
                    v-model="currentPage"
                    :item-count="filteredFiles.length"
                    :items-per-page="20"
                />
            </div>
        </div>
    </DashboardContent>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';
import { upperFirst } from 'scule';

import { UiButton } from '#components';

const stats = useStats();
const embed = useEmbed();
const files = useFiles();
const folders = useFolders();
const currentUser = useAuthUser();
const { $toast } = useNuxtApp();

const greetingMessage = computed(() => {
    const hour = new Date().getHours();

    if (hour >= 5 && hour < 12) {
        return 'Good Morning';
    } else if (hour >= 12 && hour < 17) {
        return 'Good Afternoon';
    } else if (hour >= 17 && hour < 22) {
        return 'Good Evening';
    } else {
        return 'Good Night';
    }
});

const currentPage = ref(1);

const willBeDeleted = ref(new Set<string>());
const copiedFiles = ref(new Set<string>());

const isLoading = ref(files.value.length !== currentUser.value!.stats.files || !stats.value);

const viewModal = reactive({
    open: false,
    fileId: null as string | null,
});
const editModal = reactive({
    open: false,
    file: null as FileData | null,
});

const filteredFiles = computed(() => files.value.filter((f) => !f.folderId));

const calculatedFiles = computed(() => {
    const start = (currentPage.value - 1) * 20;
    const end = start + 20;
    return filteredFiles.value.slice(start, end);
});

const handleCopy = async (file: FileData) => {
    const { copy, copied } = useClipboard({ legacy: true });

    copy(embed.value.enabled ? file.embedUrl : file.directUrl);

    $toast.success('Link copied to clipboard');

    copiedFiles.value.add(file.fileName);

    const unwatch = watch(copied, () => {
        copiedFiles.value.delete(file.fileName);
        unwatch();
    });
};

const handleDelete = async (id: string) => {
    willBeDeleted.value.add(id);

    try {
        await $fetch(`/api/files/${id}`, {
            method: 'DELETE',
        });
        $toast.success('File deleted successfully');
    } catch (error: any) {
        $toast.error(error.data.message);
    }

    willBeDeleted.value.delete(id);
};

const { all, enter: _enter, leave: _leave } = animateCards();

onMounted(async () => {
    if (isLoading.value) {
        const filesData = await $fetch('/api/files');
        const foldersData = await $fetch('/api/folders');
        const statsData = await $fetch('/api/users/@me/stats');

        files.value = filesData.map((f) => ({
            ...f,
            expiresAt: f.expiresAt ? new Date(f.expiresAt) : null,
            createdAt: new Date(f.createdAt),
        }));

        folders.value = foldersData.map((f) => ({
            ...f,
            createdAt: new Date(f.createdAt),
        }));

        stats.value = statsData;
    }

    isLoading.value = false;

    await nextTick();

    all('recentFiles', '.fileCard');
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'user-only',
});
</script>
