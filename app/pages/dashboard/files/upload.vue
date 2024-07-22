<template>
    <div>
        <Head>
            <Title>Upload Files</Title>
        </Head>

        <UiModal v-model="settingsModalOpen" p8 space-y-4>
            <h2>Settings</h2>

            <div space-y-1>
                <UiLabel :for="id">File Name Type</UiLabel>

                <UiTabs
                    :id
                    v-model="settings.fileNameType"
                    :items="[
                        {
                            label: 'Random',
                            icon: 'heroicons-solid:cube',
                        },
                        {
                            label: 'UUID',
                            icon: 'heroicons-solid:key',
                        },
                        {
                            label: 'Original',
                            icon: 'heroicons-solid:document',
                        },
                    ]"
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
                caption="Set to 0 for unlimited views"
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
            <CompressionPicker v-model="settings.compression">
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
        </UiModal>

        <div space-y-6>
            <div flex="~ items-center justify-between">
                <h2>Upload Files</h2>
                <UiButton
                    icon="heroicons-solid:document-duplicate"
                    icon-size="20"
                    alignment="center"
                    variant="accent"
                    h8
                    w8
                    p0="!"
                    href="/dashboard/files"
                    aria-label="View all files"
                />
            </div>

            <DropZone v-model="uploadingFiles" :disabled="uploading" />

            <div flex="~ gap2 items-center">
                <UiButton
                    wfull
                    gap2
                    alignment="center"
                    variant="accent"
                    icon="heroicons-solid:upload"
                    icon-size="20"
                    :disabled="uploading || !uploadingFiles.length"
                    :loading="uploading"
                    @click="handleUpload"
                >
                    Upload File(s)
                </UiButton>
                <UiButton
                    h10
                    w10
                    p0="!"
                    alignment="center"
                    icon="heroicons-solid:cog"
                    icon-size="20"
                    :disabled="uploading || !uploadingFiles.length"
                    @click="settingsModalOpen = true"
                />
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
import { toast } from 'vue-sonner';

const router = useRouter();

const uploading = useIsUploading();
const uploadingFiles = useUploadingFiles();

const settingsModalOpen = ref(false);

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
});

const id = useId();

let {
    public: { fileChunkSize },
} = useRuntimeConfig();

fileChunkSize = (+fileChunkSize || 10) * 1024 * 1024;

const handleUpload = async () => {
    uploading.value = true;

    for (const file of uploadingFiles.value) {
        const chunks = Math.ceil(file.size / fileChunkSize);

        for (let i = 0; i < chunks; i++) {
            const start = i * fileChunkSize;
            const end = Math.min(file.size, start + fileChunkSize);

            const chunk = file.slice(start, end);

            const formData = new FormData();

            formData.append(
                'file',
                new Blob([chunk], { type: file.type }),
                file.name,
            );
            formData.append('currentChunk', (i + 1).toString());
            formData.append('totalChunks', chunks.toString());
            formData.append('fileNameType', settings.fileNameType.toString());
            formData.append('maxViews', settings.maxViews.toString());
            formData.append(
                'compression',
                settings.compression.value.toString(),
            );

            if (settings.password) {
                formData.append('password', settings.password.toString());
            }

            if (settings.expiration.value) {
                formData.append(
                    'expiration',
                    settings.expiration.value!.toString(),
                );
            }

            const uploadingFile = uploadingFiles.value.find(
                (f) => f.name === file.name,
            )!;

            if (!uploadingFile) continue;

            if (!uploadingFile.status) {
                uploadingFile.status = reactive({
                    started: false,
                    progress: 0,
                    error: null,
                });
            }

            uploadingFile.status!.started = true;

            try {
                await $fetch('/api/files', {
                    method: 'POST',
                    body: formData,
                    retry: 3,
                });

                uploadingFile.status!.progress = Math.round(
                    (end / file.size) * 100,
                );

                uploadingFile.status!.error = null;
            } catch (error: any) {
                uploadingFile.status!.error = error.data.message;
                break;
            }
        }
    }

    uploadingFiles.value = uploadingFiles.value.filter(
        (file) => file.status?.error,
    );
    uploading.value = false;

    if (!uploadingFiles.value.length) {
        toast.success('All file(s) uploaded successfully');
    } else {
        toast.error('Some file(s) failed to upload');
    }
};

router.beforeEach((_, __, next) => {
    if (!uploading.value) uploadingFiles.value = [];

    next();
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'only-file-uploader',
});
</script>
