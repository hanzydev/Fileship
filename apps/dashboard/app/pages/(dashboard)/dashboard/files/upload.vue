<template>
    <Head>
        <Title>Upload Files</Title>
    </Head>

    <UiModal v-model="settingsModalOpen" p8 space-y-4>
        <h2>Upload Settings</h2>

        <div v-if="currentTab === 'Media Upload'" space-y-1>
            <UiLabel :for="id">File Name Type</UiLabel>

            <UiTabs
                :id
                v-model="settings.fileNameType"
                variant="secondary"
                :items="[
                    {
                        label: 'Random',
                        icon: 'solar:box-bold',
                    },
                    {
                        label: 'UUID',
                        icon: 'solar:key-bold',
                    },
                    {
                        label: 'Original',
                        icon: 'solar:document-bold',
                    },
                ]"
                rounded-xl="!"
                button-class="rounded-lg!"
                width-full
            />
        </div>

        <UiInput
            v-model="settings.password!"
            wfull
            label="Password"
            type="password"
            :disabled="uploading"
        />
        <UiInput
            v-model="settings.maxViews"
            wfull
            label="Max Views"
            caption="Set to 0 for unlimited views."
            type="number"
            :min="0"
            :disabled="uploading"
        />
        <ExpirationPicker v-model="settings.expiration">
            <UiInput
                v-model="settings.expiration.label"
                label="Expiration"
                type="string"
                :disabled="uploading"
                readonly
                wfull
                cursor-pointer="!"
            />
        </ExpirationPicker>
        <CompressionPicker v-if="currentTab === 'Media Upload'" v-model="settings.compression">
            <UiInput
                v-model="settings.compression.label"
                label="Compression"
                type="string"
                :disabled="uploading"
                readonly
                wfull
                cursor-pointer="!"
            />
        </CompressionPicker>
        <FolderPicker v-model="settings.folder">
            <UiInput
                v-model="settings.folder.label"
                label="Folder"
                type="string"
                :disabled="uploading"
                readonly
                wfull
                cursor-pointer="!"
            />
        </FolderPicker>
    </UiModal>

    <DashboardContent>
        <template #header>
            <UiButton
                icon="solar:arrow-left-linear"
                icon-size="20"
                alignment="center"
                size-10
                gap-2
                href="/dashboard/files"
            />
            <h2 lt-md="text-2xl!" mra>Upload Files</h2>
        </template>

        <UiTabs
            v-model="currentTab"
            :items="[
                {
                    label: 'Media Upload',
                    icon: 'solar:gallery-bold',
                },
                {
                    label: 'Text Upload',
                    icon: 'solar:document-text-bold',
                },
            ]"
            width-full
            rounded-xl="!"
            button-class="rounded-lg!"
            :disabled="uploading"
        />

        <div wfull rounded-xl bg-fs-overlay-2 p6 space-y-6 border="~ fs-overlay-4">
            <h3>
                <template v-if="currentTab === 'Media Upload'">Upload Media Files</template>
                <template v-else>Create Text File</template>
            </h3>

            <DropZone
                v-if="currentTab === 'Media Upload'"
                v-model="uploadingFiles"
                :disabled="uploading"
            />

            <div v-else space-y-4>
                <div flex="~ items-center gap4 <sm:col" wfull>
                    <UiInput
                        v-model="textFileData.fileName"
                        label="File Name"
                        type="text"
                        required
                        wfull
                        flex-1
                        wrapper-class="wfull"
                        :disabled="uploading"
                    />
                    <FileTypePicker v-model="textFileData.fileType">
                        <UiInput
                            v-model="textFileData.fileType.label"
                            label="File Type"
                            type="string"
                            :disabled="uploading"
                            cursor-pointer="!"
                            readonly
                            required
                            wfull
                            wrapper-class="wfull sm:w72"
                        />
                    </FileTypePicker>
                </div>

                <UiTextArea
                    v-model="textFileData.content"
                    :disabled="uploading"
                    label="Text"
                    required
                    wfull
                />
            </div>

            <div flex="~ gap2 items-center">
                <UiButton
                    wfull
                    gap2
                    alignment="center"
                    variant="accent"
                    icon="solar:upload-minimalistic-linear"
                    icon-size="20"
                    :disabled="
                        uploading ||
                        (!uploadingFiles.length &&
                            (!textFileData.content || !textFileData.fileName))
                    "
                    :loading="uploading"
                    @click="handleUpload"
                >
                    <template v-if="currentTab === 'Media Upload'">Upload File(s)</template>
                    <template v-else>Create Text</template>
                </UiButton>
                <UiButton
                    h10
                    w10
                    p0="!"
                    alignment="center"
                    icon="solar:settings-minimalistic-bold"
                    icon-size="20"
                    variant="secondary"
                    flex-shrink-0
                    :disabled="
                        uploading ||
                        (!uploadingFiles.length &&
                            (!textFileData.content || !textFileData.fileName))
                    "
                    @click="settingsModalOpen = true"
                />
            </div>
        </div>
    </DashboardContent>
</template>

<script setup lang="ts">
const router = useRouter();
const folders = useFolders();
const { $toast } = useNuxtApp();

const { data: foldersData } = await useFetch('/api/folders');

folders.value = foldersData.value!.map((f) => ({
    ...f,
    createdAt: new Date(f.createdAt),
}));

const uploading = useIsUploading();
const uploadingFiles = useUploadingFiles();

const settingsModalOpen = ref(false);
const currentTab = ref('Media Upload');

const textFileData = reactive({
    fileName: '',
    fileType: TEXT_FILE_TYPES[0]!,
    content: '',
});

const settings = reactive<{
    fileNameType: 'Random' | 'UUID' | 'Original';
    maxViews: number;
    password: string | null;
    expiration: {
        label: string;
        value: number | null;
    };
    compression: {
        label: string;
        value: number;
    };
    folder: {
        label: string;
        value: string | null;
    };
}>({
    fileNameType: 'Random',
    maxViews: 0,
    password: null,
    expiration: {
        label: 'Never',
        value: null,
    },
    compression: {
        label: 'None',
        value: 0,
    },
    folder: {
        label: 'None',
        value: null,
    },
});

const id = useId();

const handleUpload = async () => {
    uploading.value = true;

    if (currentTab.value === 'Text Upload') {
        settings.fileNameType = 'Original';

        uploadingFiles.value.push(
            new File(
                [textFileData.content],
                `${textFileData.fileName}.${textFileData.fileType.extension}`,
                {
                    type: textFileData.fileType.mime,
                },
            ),
        );
    }

    const results: boolean[] = [];
    const parallelUploads = 3;
    const files = [...uploadingFiles.value];

    while (files.length > 0) {
        const chunk = files.splice(0, parallelUploads);
        const chunkResults = await Promise.all(
            chunk.map((c) =>
                uploadFile(c, {
                    fileNameType: settings.fileNameType,
                    maxViews: settings.maxViews,
                    password: settings.password,
                    expiration: settings.expiration.value,
                    compression: settings.compression.value,
                    folder: settings.folder.value,
                }),
            ),
        );
        results.push(...chunkResults.filter((r) => r !== undefined));
    }

    uploadingFiles.value = uploadingFiles.value.filter((_, index) => !results[index]);
    uploading.value = false;

    if (!uploadingFiles.value.length) {
        $toast.success('All files uploaded successfully');

        textFileData.fileName = '';
        textFileData.content = '';
        textFileData.fileType = TEXT_FILE_TYPES[0]!;
    } else {
        $toast.error('Some files could not be uploaded');
    }
};

router.beforeEach((_, __, next) => {
    if (!uploading.value) uploadingFiles.value = [];

    next();
});

watch(currentTab, () => {
    uploadingFiles.value = [];

    textFileData.fileName = '';
    textFileData.content = '';
    textFileData.fileType = TEXT_FILE_TYPES[0]!;
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'file-uploader-only',
});
</script>
