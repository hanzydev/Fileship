<template>
    <div class="flex min-h-svh flex-col">
        <Head>
            <Title v-if="!data.id">{{ $t('publicPages.inbox.verifyPassword') }}</Title>
            <Title v-else>{{ title }}</Title>
        </Head>

        <PasswordGate
            v-if="!data.id"
            :title="$t('publicPages.inbox.passwordProtected')"
            :disabled="isVerifying"
            :error="passwordError"
            @submit="handlePassword"
        />

        <template v-else>
            <ViewHeader
                :title="title"
                :subtitle="$t('publicPages.inbox.subtitle')"
                content-class="max-w-lg"
            />

            <main class="flex flex-1 items-center justify-center p-4">
                <UiCard class="w-full max-w-lg">
                    <UiCardHeader>
                        <div class="flex items-center gap-3">
                            <UiAvatar>
                                <UiAvatarImage v-if="data.user!.avatar" :src="data.user!.avatar" />
                                <UiAvatarFallback>
                                    {{ data.user!.username.slice(0, 2).toUpperCase() }}
                                </UiAvatarFallback>
                            </UiAvatar>
                            <div class="flex flex-col">
                                <UiCardTitle>{{ $t('publicPages.inbox.sendFiles') }}</UiCardTitle>
                                <UiCardDescription>
                                    {{
                                        $t('publicPages.inbox.sendFilesDescription', {
                                            name: upperFirst(data.user!.username),
                                        })
                                    }}
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
                            <UiItem
                                v-for="(file, index) in uploadingFiles"
                                :key="`${file.name}-${index}`"
                                variant="muted"
                                size="sm"
                            >
                                <UiItemMedia variant="icon">
                                    <IFile />
                                </UiItemMedia>
                                <UiItemContent class="min-w-0">
                                    <UiItemTitle class="max-w-full">
                                        <span class="truncate">{{ file.name }}</span>
                                    </UiItemTitle>
                                    <UiItemDescription
                                        :class="file.status?.error && 'text-destructive'"
                                    >
                                        {{ file.status?.error || filesize(file.size, { locale }) }}
                                    </UiItemDescription>
                                </UiItemContent>
                                <UiItemActions>
                                    <UiButton
                                        v-if="!file.status?.started"
                                        variant="ghost"
                                        size="icon-sm"
                                        :aria-label="$t('publicPages.inbox.remove')"
                                        @click="uploadingFiles.splice(index, 1)"
                                    >
                                        <IX />
                                    </UiButton>
                                    <ICheck
                                        v-else-if="file.status.completed"
                                        class="size-4 text-primary"
                                    />
                                    <UiItemDescription v-else-if="!file.status.error">
                                        {{
                                            $t('publicPages.inbox.percent', {
                                                percent: file.status.progress.percent,
                                            })
                                        }}
                                    </UiItemDescription>
                                </UiItemActions>
                                <UiItemFooter
                                    v-if="
                                        file.status?.started &&
                                        !file.status.completed &&
                                        !file.status.error
                                    "
                                >
                                    <UiProgress
                                        class="flex-1"
                                        :model-value="file.status.progress.percent"
                                    />
                                </UiItemFooter>
                            </UiItem>
                        </div>

                        <UiAlert v-if="isDone">
                            <ICircleCheck />
                            <UiAlertTitle>{{ $t('publicPages.inbox.filesSent') }}</UiAlertTitle>
                            <UiAlertDescription>
                                {{
                                    $t('publicPages.inbox.filesReceived', {
                                        name: upperFirst(data.user!.username),
                                    })
                                }}
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
                                    ? $t('publicPages.inbox.uploading', { progress })
                                    : uploadingFiles.length
                                      ? $t('publicPages.inbox.sendCount', uploadingFiles.length)
                                      : $t('publicPages.inbox.sendFiles')
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
const { t, locale } = useI18n();

const id = (route.params as any).id as string;
const { data: _data, error } = await useFetch(
    `/api/inbox/${id}${route.query.password ? `?password=${encodeURIComponent(String(route.query.password))}` : ''}`,
);

if (error.value?.status === 404) {
    throw createError({ status: 404, message: t('publicPages.inbox.notFound') });
}

const data = ref<any>({
    ..._data.value,
    password: route.query.password || null,
});

const title = computed(() =>
    t('publicPages.inbox.title', { name: upperFirst(data.value.user!.username) }),
);

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
    uploadingFiles.value = mergeFiles(uploadingFiles.value, files);
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
        $toast.error(t('publicPages.inbox.someFailed'));
    } else {
        isDone.value = true;
        $toast.success(t('publicPages.inbox.allUploaded'));
    }
};

onUnmounted(() => {
    if (!uploading.value) uploadingFiles.value = [];
});
</script>
