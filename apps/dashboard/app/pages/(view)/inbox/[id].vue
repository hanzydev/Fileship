<template>
    <div>
        <Head>
            <Title v-if="!data.id">Verify Password</Title>
            <Title v-else>{{ upperFirst(data.user!.username) }}'s Inbox</Title>
        </Head>

        <UiCentered hfull>
            <VerifyPassword
                v-if="!data.id"
                :disabled="passwordDisabled"
                :error="passwordError"
                @password="handlePassword"
            />
            <div v-else max-w-3xl wfull px-4 space-y-4>
                <section
                    flex="~ items-center gap-4"
                    border="~ fs-overlay-3"
                    h5rem
                    rounded-2xl
                    bg-fs-overlay-1
                    px-6
                >
                    <UiAvatar :src="data.user!.avatar!" :alt="data.user!.username" size="sm" />
                    <h3>{{ upperFirst(data.user!.username) }}'s Inbox</h3>
                </section>

                <div wfull rounded-2xl bg-fs-overlay-1 p6 space-y-6 border="~ fs-overlay-4">
                    <DropZone v-model="uploadingFiles" variant="secondary" :disabled="uploading" />

                    <div v-if="uploadingFiles.length > 0" text-left space-y-4>
                        <div v-bind="containerProps" max-h25rem>
                            <div flex="~ items-center" wfull rounded-xl bg-fs-overlay-3 px3 py2>
                                <h6 w="2.5/4">File</h6>
                                <h6 w="1.2/4">Size</h6>
                                <h6 w="1.5/4">Status</h6>
                                <h6 w="3.7/4">Progress</h6>
                            </div>

                            <div v-bind="wrapperProps">
                                <UiDropdown
                                    v-for="{ data: file } in list"
                                    :key="file.name"
                                    as-ctx-menu
                                >
                                    <div flex="~ items-center" mx3 h10>
                                        <span
                                            line-clamp-1
                                            w="2.5/4"
                                            break-all
                                            text-sm="!"
                                            font-medium="!"
                                        >
                                            {{ file.name }}
                                        </span>
                                        <span w="1.2/4" text-sm="!" font-medium="!">
                                            {{ filesize(file.size) }}
                                        </span>
                                        <div
                                            flex="~ items-center gap2"
                                            w="1.5/4"
                                            text-sm="!"
                                            font-medium="!"
                                        >
                                            <template v-if="file.status?.error">
                                                <Icon
                                                    name="heroicons:exclamation-circle-solid"
                                                    size="16"
                                                    flex-shrink-0
                                                    text-red-500
                                                />
                                                {{ file.status?.error }}
                                            </template>
                                            <template v-else-if="file.status?.completed">
                                                <Icon
                                                    name="solar:check-circle-bold"
                                                    size="16"
                                                    flex-shrink-0
                                                    text-fs-accent
                                                />
                                                Finished
                                            </template>
                                            <template v-else-if="!file.status?.started">
                                                <Icon
                                                    name="solar:alarm-bold"
                                                    size="16"
                                                    flex-shrink-0
                                                    text-yellow-500
                                                />
                                                Queued
                                            </template>

                                            <template v-else-if="file.status?.progress?.percent">
                                                <Icon
                                                    name="solar:upload-minimalistic-linear"
                                                    size="16"
                                                    flex-shrink-0
                                                />
                                                Uploading
                                            </template>
                                            <template v-else-if="file.status?.started">
                                                <Icon
                                                    name="solar:rocket-2-bold"
                                                    size="16"
                                                    flex-shrink-0
                                                    text-green-500
                                                />
                                                Starting
                                            </template>
                                        </div>
                                        <div flex="~ items-center gap-1" w="3.7/4">
                                            <template
                                                v-if="
                                                    file.status?.progress?.speed &&
                                                    !file.status?.error
                                                "
                                            >
                                                <span mra text-sm>
                                                    {{ filesize(file.status.progress.speed) }}/s
                                                </span>
                                                <span>|</span>
                                                <span mxa text-sm>
                                                    ETA:
                                                    {{
                                                        dayjs
                                                            .duration(
                                                                file.status.progress.eta,
                                                                'seconds',
                                                            )
                                                            .format('HH:mm:ss')
                                                    }}
                                                </span>
                                                <span>|</span>
                                            </template>
                                            <UiProgress
                                                :value="file.status?.progress?.percent || 0"
                                                :class="
                                                    file.status?.progress?.speed &&
                                                    !file.status?.error
                                                        ? 'mla'
                                                        : 'mra'
                                                "
                                            />
                                        </div>
                                    </div>
                                    <template #content>
                                        <div
                                            w40
                                            rounded-xl
                                            bg-fs-overlay-2
                                            p1
                                            ring="1 fs-overlay-4"
                                        >
                                            <UiButton
                                                variant="onOverlay"
                                                wfull
                                                gap2
                                                text-red-500
                                                py1="!"
                                                icon="lucide:x"
                                                icon-size="20"
                                                :disabled="
                                                    !file.status?.error && file.status?.started
                                                "
                                                @click="
                                                    uploadingFiles = uploadingFiles.filter(
                                                        (f) => f !== file,
                                                    )
                                                "
                                            >
                                                Remove
                                            </UiButton>
                                        </div>
                                    </template>
                                </UiDropdown>
                            </div>
                        </div>
                    </div>

                    <UiButton
                        wfull
                        gap2
                        alignment="center"
                        variant="accent"
                        icon="solar:upload-minimalistic-linear"
                        :disabled="uploading || !uploadingFiles.length"
                        :loading="uploading"
                        @click="handleUpload"
                    >
                        {{ uploading ? `Uploading (${progress}%)` : 'Upload File(s)' }}
                    </UiButton>
                </div>
            </div>
        </UiCentered>
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';
import { filesize } from 'filesize';
import { upperFirst } from 'scule';

const route = useRoute();
const uploading = useIsUploading();
const uploadingFiles = useUploadingFiles();
const { $toast } = useNuxtApp();

const { data: _data, error } = await useFetch(
    `/api/inbox/${route.params.id}${route.query.password ? `?password=${route.query.password}` : ''}`,
);

const data = ref({
    ..._data.value,
    password: route.query.password || null,
});

if (error.value?.statusCode === 404) {
    throw createError({
        statusCode: 404,
        message: 'Inbox not found',
    });
}

const passwordError = ref<string>();
const passwordDisabled = ref(false);

const progress = computed(() =>
    clamp(
        Math.round(
            uploadingFiles.value.reduce(
                (acc, file) => acc + (file.status?.progress?.percent || 0),
                0,
            ) / uploadingFiles.value.length,
        ),
        0,
        100,
    ),
);

const { list, containerProps, wrapperProps } = useVirtualList(uploadingFiles, {
    itemHeight: 40,
});

const handlePassword = async (password: string) => {
    passwordError.value = undefined;
    passwordDisabled.value = true;

    try {
        const inbox = await $fetch(`/api/inbox/${route.params.id}`, {
            query: {
                password,
            },
        });

        data.value = {
            ...inbox,
            password,
        };
    } catch (error: any) {
        passwordError.value = error.data.message;
        passwordDisabled.value = false;
    }
};

const handleUpload = async () => {
    uploading.value = true;

    uploadingFiles.value.forEach((file) => {
        if (!file.status) {
            file.status = reactive({
                started: false,
                completed: false,
                progress: { speed: 0, percent: 0, eta: 0 },
                error: null,
            });
        }
    });

    const parallelUploads = 3;
    const files = [...uploadingFiles.value];

    const uploadResults = await uploadFilesWithConcurrency(
        files,
        (file) =>
            uploadFile(
                file,
                {
                    inboxPassword: data.value.password as string,
                },
                `/api/inbox/${data.value.id}/upload`,
            ),
        parallelUploads,
    );
    const results = uploadResults.map((result) => typeof result === 'string');

    uploadingFiles.value = uploadingFiles.value.filter((_, index) => !results[index]);
    uploading.value = false;

    if (!uploadingFiles.value.length) {
        $toast.success('All files uploaded successfully');
    } else {
        $toast.error('Some files could not be uploaded');
    }
};

onUnmounted(() => {
    if (!uploading.value) uploadingFiles.value = [];
});
</script>
