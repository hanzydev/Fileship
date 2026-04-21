<template>
    <Head>
        <Title>Upload Files</Title>
    </Head>

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

        <div grid="~ cols-1 lg:cols-3 gap6" wfull>
            <div lg:col-span-2>
                <UploadZoneSection
                    ref="uploadZoneSectionRef"
                    :active-tab="currentTab"
                    :disabled="uploading"
                    @upload="handleUpload"
                />
            </div>

            <div lg:col-span-1>
                <div border="~ fs-overlay-4" sticky top-6 wfull rounded-xl bg-fs-overlay-2 p6>
                    <UploadSettings
                        :settings="settings"
                        :disabled="uploading"
                        :show-file-name-type="currentTab === 'Media Upload'"
                        :show-compression="currentTab === 'Media Upload'"
                        @update:file-name-type="(val) => (settings.fileNameType = val)"
                        @update:password="(val) => (settings.password = val)"
                        @update:max-views="(val) => (settings.maxViews = val)"
                        @update:expiration="(val) => (settings.expiration = val)"
                        @update:folder="(val) => (settings.folder = val)"
                        @update:compression="(val) => (settings.compression = val)"
                    />
                </div>
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

const currentTab = ref<'Media Upload' | 'Text Upload'>('Media Upload');

const uploadZoneSectionRef = ref<any>(null);

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

const handleUpload = async () => {
    uploading.value = true;

    const textFileData = uploadZoneSectionRef.value?.textFileData;

    if (currentTab.value === 'Text Upload' && textFileData) {
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

    uploadingFiles.value.forEach((file) => {
        if (!file.status) {
            file.status = reactive({
                started: false,
                progress: { speed: 0, percent: 0, eta: 0 },
                error: null,
            });
        }
    });

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

        if (textFileData) {
            textFileData.fileName = '';
            textFileData.content = '';
            textFileData.fileType = TEXT_FILE_TYPES[0]!;
        }
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

    const textFileData = uploadZoneSectionRef.value?.textFileData;
    if (textFileData) {
        textFileData.fileName = '';
        textFileData.content = '';
        textFileData.fileType = TEXT_FILE_TYPES[0]!;
    }
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'file-uploader-only',
});
</script>
