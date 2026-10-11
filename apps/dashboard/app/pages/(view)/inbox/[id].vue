<template>
    <div class="flex min-h-svh flex-col">
        <Head>
            <Title v-if="!data.id">Verify Password</Title>
            <Title v-else>{{ upperFirst(data.user!.username) }}'s Inbox</Title>
        </Head>

        <PasswordGate
            v-if="!data.id"
            title="This inbox is password protected"
            :disabled="isVerifying"
            :error="passwordError"
            @submit="handlePassword"
        />

        <template v-else>
            <ViewHeader
                :title="`${upperFirst(data.user!.username)}'s Inbox`"
                subtitle="Send files directly to their account"
                content-class="max-w-3xl"
            />

            <main class="mx-auto flex w-full max-w-3xl flex-1 flex-col p-4">
                <UiCard>
                    <UiCardHeader>
                        <div class="flex items-center gap-3">
                            <UiAvatar>
                                <UiAvatarImage v-if="data.user!.avatar" :src="data.user!.avatar" />
                                <UiAvatarFallback>
                                    {{ data.user!.username.slice(0, 2).toUpperCase() }}
                                </UiAvatarFallback>
                            </UiAvatar>
                            <div class="flex flex-col">
                                <UiCardTitle>Send files</UiCardTitle>
                                <UiCardDescription>
                                    Files you upload go straight to
                                    {{ upperFirst(data.user!.username) }}'s inbox.
                                </UiCardDescription>
                            </div>
                        </div>
                    </UiCardHeader>
                    <UiCardContent class="flex flex-col gap-4">
                        <DropZone :disabled="uploading" @files="addFiles" />

                        <div
                            v-if="uploadingFiles.length"
                            class="flex max-h-72 flex-col gap-2 overflow-y-auto"
                        >
                            <UiCard
                                v-for="(file, index) in uploadingFiles"
                                :key="`${file.name}-${index}`"
                                size="sm"
                            >
                                <UiCardContent class="flex flex-col gap-1.5">
                                    <div class="flex items-center gap-2">
                                        <TextBlock variant="small" class="flex-1 truncate">
                                            {{ file.name }}
                                        </TextBlock>
                                        <TextBlock variant="muted" class="shrink-0 text-xs">
                                            {{ filesize(file.size) }}
                                        </TextBlock>
                                        <UiButton
                                            v-if="!file.status?.started"
                                            variant="ghost"
                                            size="icon-xs"
                                            @click="uploadingFiles.splice(index, 1)"
                                        >
                                            <IX />
                                        </UiButton>
                                        <ICheck
                                            v-else-if="file.status.completed"
                                            class="size-4 text-primary"
                                        />
                                    </div>
                                    <UiProgress
                                        v-if="file.status?.started"
                                        :model-value="file.status.progress.percent"
                                    />
                                    <UiFieldError
                                        v-if="file.status?.error"
                                        :errors="[file.status.error]"
                                    />
                                </UiCardContent>
                            </UiCard>
                        </div>

                        <UiAlert v-if="isDone">
                            <ICircleCheck />
                            <UiAlertTitle>Files sent</UiAlertTitle>
                            <UiAlertDescription>
                                {{ upperFirst(data.user!.username) }} has received your files.
                            </UiAlertDescription>
                        </UiAlert>
                    </UiCardContent>
                    <UiCardFooter>
                        <UiButton
                            class="w-full"
                            :disabled="!uploadingFiles.length || uploading"
                            @click="handleUpload"
                        >
                            <UiSpinner v-if="uploading" />
                            <ISend v-else />
                            {{
                                uploading
                                    ? `Uploading (${progress}%)`
                                    : uploadingFiles.length
                                      ? `Send ${pluralize(uploadingFiles.length, 'file')}`
                                      : 'Send files'
                            }}
                        </UiButton>
                    </UiCardFooter>
                </UiCard>
            </main>
        </template>
    </div>
</template>

<script setup lang="ts">
import { filesize } from 'filesize';
import { upperFirst } from 'scule';

const route = useRoute();
const uploading = useIsUploading();
const uploadingFiles = useUploadingFiles();
const { $toast } = useNuxtApp();

const id = (route.params as any).id as string;
const { data: _data, error } = await useFetch(
    `/api/inbox/${id}${route.query.password ? `?password=${encodeURIComponent(String(route.query.password))}` : ''}`,
);

if (error.value?.statusCode === 404) {
    throw createError({ statusCode: 404, message: 'Inbox not found' });
}

const data = ref<any>({
    ..._data.value,
    password: route.query.password || null,
});

const passwordError = ref<string>();
const isVerifying = ref(false);
const isDone = ref(false);

const progress = computed(() =>
    clamp(
        Math.round(
            uploadingFiles.value.reduce(
                (acc, file) => acc + (file.status?.progress.percent || 0),
                0,
            ) / uploadingFiles.value.length,
        ),
        0,
        100,
    ),
);

const addFiles = (files: File[] | null) => {
    if (!files?.length) return;

    isDone.value = false;
    uploadingFiles.value = [
        ...uploadingFiles.value,
        ...files.filter((f) => !uploadingFiles.value.some((file) => file.name === f.name)),
    ];
};

const handlePassword = async (password: string) => {
    passwordError.value = undefined;
    isVerifying.value = true;

    try {
        const inbox = await $fetch(`/api/inbox/${id}`, { query: { password } });
        data.value = { ...inbox, password };
    } catch (error: any) {
        passwordError.value = getErrorMessage(error);
    }

    isVerifying.value = false;
};

const handleUpload = async () => {
    uploading.value = true;

    const files = [...uploadingFiles.value];

    const results = await uploadFilesWithConcurrency(files, (file) =>
        uploadFile(
            file,
            { inboxPassword: data.value.password },
            `/api/inbox/${data.value.id}/upload`,
        ),
    );

    uploadingFiles.value = files.filter((_, index) => typeof results[index] !== 'string');
    uploading.value = false;

    if (uploadingFiles.value.length) {
        $toast.error('Some files could not be uploaded');
    } else {
        isDone.value = true;
        $toast.success('All files uploaded successfully');
    }
};

onUnmounted(() => {
    if (!uploading.value) uploadingFiles.value = [];
});
</script>
