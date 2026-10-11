<template>
    <Head>
        <Title>{{ folder?.name || 'Folder' }}</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-6 p-4 pt-0">
        <EmptyState
            v-if="!folder && !isLoading"
            :icon="IFolderX"
            title="Folder not found"
            description="It may have been deleted."
        >
            <UiButton variant="outline" as-child>
                <NuxtLink to="/dashboard/folders">Back to folders</NuxtLink>
            </UiButton>
        </EmptyState>

        <template v-else-if="folder">
            <PageHeader :title="folder.name" :description="description">
                <UiButton variant="outline" @click="addFilesOpen = true">
                    <IFilePlus />
                    Add files
                </UiButton>
                <UiButton v-if="folder.public" variant="outline" @click="copyLink(folder)">
                    <ILink />
                    Copy link
                </UiButton>
                <UiDropdownMenu>
                    <UiDropdownMenuTrigger as-child>
                        <UiButton variant="outline" size="icon">
                            <IEllipsis />
                        </UiButton>
                    </UiDropdownMenuTrigger>
                    <UiDropdownMenuContent align="end" class="w-56">
                        <FolderMenuItems
                            :folder="folder"
                            type="dropdown"
                            in-folder
                            @edit="formOpen = true"
                        />
                    </UiDropdownMenuContent>
                </UiDropdownMenu>
            </PageHeader>

            <UiAlert v-if="folder.isInbox">
                <IInbox />
                <UiAlertTitle>This is your inbox</UiAlertTitle>
                <UiAlertDescription>
                    Files that other people send you through your inbox link land here.
                    <NuxtLink to="/dashboard/settings/inbox" class="underline underline-offset-4">
                        Manage inbox settings
                    </NuxtLink>
                </UiAlertDescription>
            </UiAlert>

            <FileBrowser
                :files="folderFiles"
                :search-endpoint="`/api/folders/${folder.id}/searchFiles`"
                :is-loading="isLoading"
                empty-title="This folder is empty"
            />

            <FolderFormDialog v-model:open="formOpen" :folder="folder" />

            <UiDialog v-model:open="addFilesOpen">
                <UiDialogContent class="sm:max-w-lg">
                    <UiDialogHeader>
                        <UiDialogTitle>Add files to {{ folder.name }}</UiDialogTitle>
                        <UiDialogDescription>Pick files from your library.</UiDialogDescription>
                    </UiDialogHeader>
                    <UiInputGroup>
                        <UiInputGroupAddon>
                            <ISearch />
                        </UiInputGroupAddon>
                        <UiInputGroupInput v-model="addSearch" placeholder="Search files..." />
                    </UiInputGroup>
                    <div class="-mx-2 flex max-h-80 flex-col gap-0.5 overflow-y-auto px-2">
                        <label
                            v-for="file in addCandidates"
                            :key="file.id"
                            class="flex cursor-pointer items-center gap-3 rounded-xl px-2 py-1.5 hover:bg-muted"
                        >
                            <UiCheckbox
                                :model-value="addSelection.includes(file.id)"
                                @update:model-value="toggleAdd(file.id)"
                            />
                            <component
                                :is="getFileIcon(file)"
                                class="size-4 text-muted-foreground"
                            />
                            <span class="flex-1 truncate text-sm">{{ file.fileName }}</span>
                            <span class="text-sm text-muted-foreground">
                                {{ file.size.formatted }}
                            </span>
                        </label>
                        <TextBlock
                            v-if="!addCandidates.length"
                            variant="muted"
                            class="py-6 text-center"
                        >
                            No files to add.
                        </TextBlock>
                    </div>
                    <UiDialogFooter>
                        <UiButton variant="outline" @click="addFilesOpen = false">Cancel</UiButton>
                        <UiButton
                            :disabled="!addSelection.length || isAdding"
                            @click="handleAddFiles"
                        >
                            <UiSpinner v-if="isAdding" />
                            Add {{ pluralize(addSelection.length, 'file') }}
                        </UiButton>
                    </UiDialogFooter>
                </UiDialogContent>
            </UiDialog>
        </template>
    </div>
</template>

<script setup lang="ts">
import { IFolderX } from '#components';

const route = useRoute();
const files = useFiles();
const folders = useFolders();
const { $toast } = useNuxtApp();
const { copyLink } = useFolderActions();

const { status: foldersStatus } = useLazyAsyncData('folders', fetchFolders, { server: false });
const { status: filesStatus } = useLazyAsyncData('files', fetchFiles, { server: false });

const formOpen = ref(false);
const addFilesOpen = ref(false);
const addSearch = ref('');
const addSelection = ref<string[]>([]);
const isAdding = ref(false);

const folderId = computed(() => (route.params as any).id as string);
const folder = computed(() => folders.value.find((f) => f.id === folderId.value));
const folderFiles = computed(() => files.value.filter((f) => f.folderId === folderId.value));

const isLoading = computed(
    () =>
        (foldersStatus.value !== 'success' && !folders.value.length) ||
        (filesStatus.value !== 'success' && !files.value.length),
);

const description = computed(
    () =>
        `${pluralize(folderFiles.value.length, 'file')} · ${folder.value!.public ? 'Public' : 'Private'}`,
);

const addCandidates = computed(() =>
    files.value
        .filter(
            (f) =>
                f.folderId !== folderId.value &&
                f.fileName.toLowerCase().includes(addSearch.value.trim().toLowerCase()),
        )
        .slice(0, 100),
);

const toggleAdd = (id: string) => {
    addSelection.value = addSelection.value.includes(id)
        ? addSelection.value.filter((x) => x !== id)
        : [...addSelection.value, id];
};

const handleAddFiles = async () => {
    isAdding.value = true;

    try {
        await $fetch(`/api/folders/${folderId.value}`, {
            method: 'PATCH',
            body: { files: [...new Set([...folder.value!.files, ...addSelection.value])] },
        });

        $toast.success(`${pluralize(addSelection.value.length, 'file')} added`);
        addFilesOpen.value = false;
    } catch (error: any) {
        $toast.error(error.data.message);
    }

    isAdding.value = false;
};

watch(folder, (value, oldValue) => {
    if (oldValue && !value) navigateTo('/dashboard/folders');
});

watch(addFilesOpen, (value) => {
    if (value) {
        addSearch.value = '';
        addSelection.value = [];
    }
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'file-uploader-only',
});
</script>
