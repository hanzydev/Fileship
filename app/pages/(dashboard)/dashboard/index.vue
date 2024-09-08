<template>
    <div>
        <Head>
            <Title>Dashboard</Title>
        </Head>

        <ModalsViewFile
            v-if="viewModal.file"
            v-model="viewModal.open"
            :data="viewModal.file!"
        />

        <ModalsEditFile
            v-if="editModal.file"
            :key="editModal.file!.id"
            v-model="editModal.open"
            :data="editModal.file!"
        />

        <div space-y-6>
            <h2>Welcome back, {{ upperFirst(currentUser!.username) }} 👋</h2>

            <div space-y-2>
                <h3>Your Statistics</h3>

                <div grid="~ gap6 md:cols-3 sm:cols-2 xl:cols-4">
                    <StatCard
                        title="Files"
                        description="uploaded files"
                        icon="heroicons-solid:document"
                        :data="statsData!.files.count"
                        :growth="statsData!.files.growth"
                    />
                    <StatCard
                        title="Views"
                        description="total file views"
                        icon="heroicons-solid:eye"
                        :data="statsData!.views.count"
                        :growth="statsData!.views.growth"
                    />
                    <StatCard
                        title="Storage"
                        description="used storage"
                        icon="mdi:sd-storage"
                        :data="statsData!.storageUsed.size"
                        :growth="statsData!.storageUsed.growth"
                    />
                    <StatCard
                        title="Users"
                        description="total users"
                        icon="iconamoon:profile-fill"
                        :data="statsData!.users.count"
                        :growth="statsData!.users.growth"
                    />
                </div>
            </div>

            <div pt6 space-y-2>
                <h3>Recent Files</h3>
                <div
                    v-if="filteredFiles.length"
                    grid="~ gap6 lg:cols-3 md:cols-2 xl:cols-4"
                >
                    <TransitionGroup :css="false" @enter="enter" @leave="leave">
                        <div
                            v-for="file in filteredFiles.slice(0, 4)"
                            :key="file.id"
                            opacity-0
                            class="fileCard"
                        >
                            <FileCard :data="file" />
                        </div>
                    </TransitionGroup>
                </div>
                <NothingHere
                    v-else
                    message="There are no files to display."
                    icon="heroicons-solid:document-duplicate"
                />
            </div>

            <div pt6 space-y-2>
                <h3>Files</h3>
                <div space-y-4>
                    <UiTable
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
                                resolve: ({ createdAt }) =>
                                    dayjs(createdAt).fromNow(),
                            },
                            {
                                key: 'Actions',
                                width: '20%',
                                render: (row) => {
                                    const isImage =
                                        row.mimeType.startsWith('image/');

                                    const isVideo =
                                        row.mimeType.startsWith('video/');

                                    const isAudio =
                                        row.mimeType.startsWith('audio/');

                                    const canBeViewed =
                                        isImage || isVideo || isAudio;

                                    return h(
                                        'div',
                                        { class: 'flex items-center gap4' },
                                        [
                                            h(UiButton, {
                                                variant: 'outline',
                                                alignment: 'center',
                                                class: 'h8 w8 !p0 text-slate300 hover:text-white',
                                                icon: canBeViewed
                                                    ? 'heroicons:eye-16-solid'
                                                    : 'heroicons-solid:external-link',
                                                iconSize: '20',
                                                'aria-label': canBeViewed
                                                    ? 'View file'
                                                    : 'Open file in new tab',
                                                ...(canBeViewed
                                                    ? {
                                                          onClick: () => {
                                                              viewModal.file =
                                                                  row;
                                                              nextTick(
                                                                  () =>
                                                                      (viewModal.open = true),
                                                              );
                                                          },
                                                      }
                                                    : {
                                                          href: `/view/${row.fileName}`,
                                                          target: '_blank',
                                                      }),
                                            }),
                                            h(UiButton, {
                                                variant: 'outline',
                                                alignment: 'center',
                                                class: [
                                                    'h8 w8 !p0 hover:text-white',
                                                    copied.has(row.fileName) &&
                                                        'text-green500',
                                                ],
                                                icon: copied.has(row.fileName)
                                                    ? 'heroicons-solid:clipboard-check'
                                                    : 'heroicons-solid:clipboard-copy',
                                                iconSize: '20',
                                                'aria-label':
                                                    'Copy link to clipboard',
                                                onClick: () =>
                                                    handleCopy(row.fileName),
                                            }),
                                            h(UiButton, {
                                                variant: 'outline',
                                                alignment: 'center',
                                                class: 'h8 w8 !p0 text-slate300 hover:text-white',
                                                href: `/u/${row.fileName}?download`,
                                                target: '_blank',
                                                icon: 'heroicons-solid:download',
                                                iconSize: '20',
                                                'aria-label': 'Download file',
                                            }),
                                            h(UiButton, {
                                                variant: 'outline',
                                                alignment: 'center',
                                                class: 'h8 w8 !p0 text-slate300 hover:text-white',
                                                icon: 'heroicons:pencil-16-solid',
                                                iconSize: '20',
                                                'aria-label': 'Edit file',
                                                onClick: () => {
                                                    editModal.file = row;
                                                    nextTick(
                                                        () =>
                                                            (editModal.open = true),
                                                    );
                                                },
                                            }),
                                            h(UiButton, {
                                                variant: 'outline',
                                                alignment: 'center',
                                                class: 'h8 w8 !p0 ring-red-500 text-slate300 hover:text-white hover:!bg-red-500',
                                                icon: 'heroicons-solid:trash',
                                                iconSize: '20',
                                                disabled: willBeDeleted.has(
                                                    row.id,
                                                ),
                                                'aria-label': 'Delete file',
                                                onClick: () =>
                                                    handleDelete(row.id),
                                            }),
                                        ],
                                    );
                                },
                            },
                        ]"
                        :rows="calculatedFiles"
                        nothing-here-message="There are no files to display."
                        nothing-here-icon="heroicons-solid:document-duplicate"
                    />
                    <UiPagination
                        v-model="currentPage"
                        :item-count="filteredFiles.length"
                        :items-per-page="20"
                    />
                </div>
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';
import { upperFirst } from 'scule';
import { toast } from 'vue-sonner';

import { UiButton } from '#components';

const { data: foldersData } = await useFetch('/api/folders');
const { data: filesData } = await useFetch('/api/files');
const { data: statsData } = await useFetch('/api/users/@me/stats');

const embed = useEmbed();
const files = useFiles();
const folders = useFolders();
const currentUser = useAuthUser();

folders.value = foldersData.value!.map((f) => ({
    ...f,
    createdAt: new Date(f.createdAt),
}));

files.value = filesData.value!.map((f) => ({
    ...f,
    expiresAt: f.expiresAt ? new Date(f.expiresAt) : null,
    createdAt: new Date(f.createdAt),
}));

const currentPage = ref(1);

const willBeDeleted = ref(new Set<string>());
const copied = ref(new Map<string, NodeJS.Timeout>());

const viewModal = reactive({
    open: false,
    file: null as FileData | null,
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

const handleCopy = async (fileName: string) => {
    const timeout = copied.value.get(fileName);
    if (timeout) clearTimeout(timeout);

    await navigator.clipboard.writeText(
        `${useRequestURL().origin}/${embed.value.enabled ? 'view' : 'u'}/${fileName}`,
    );

    toast.success('Link copied to clipboard');

    copied.value.set(
        fileName,
        setTimeout(() => {
            copied.value.delete(fileName);
        }, 2_000),
    );
};

const handleDelete = async (id: string) => {
    willBeDeleted.value.add(id);

    try {
        await $fetch(`/api/files/${id}`, {
            method: 'DELETE',
        });
        toast.success('File deleted successfully');
    } catch (error: any) {
        toast.error(error.data.message);
    }

    willBeDeleted.value.delete(id);
};

const { all, enter, leave } = animateCards();

onMounted(() => all('recentFiles', '.fileCard'));

definePageMeta({
    layout: 'dashboard',
    middleware: 'user-only',
});
</script>
