<template>
    <div class="flex min-h-svh flex-col">
        <Head>
            <Title v-if="!data.id">Verify Password</Title>
            <Title v-else>{{ data.fileName }}</Title>

            <template v-if="data?.embed?.enabled">
                <Meta name="theme-color" :content="data.embed.color" />
                <Meta
                    property="og:title"
                    :content="replaceString(data.embed.title || data.fileName!)"
                />
                <Meta
                    v-if="data.embed.description"
                    property="og:description"
                    :content="replaceString(data.embed.description)"
                />

                <template v-if="data.embed.siteName">
                    <Meta property="og:site_name" :content="replaceString(data.embed.siteName)" />
                    <Meta name="twitter:site" :content="replaceString(data.embed.siteName)" />
                </template>
            </template>

            <template v-if="isImage">
                <Meta property="og:type" content="image" />
                <Meta property="og:image" itemprop="image" :content="fileUrl" />
                <Meta property="og:url" :content="fileUrl" />

                <Meta name="twitter:title" :content="data.fileName" />
                <Meta name="twitter:card" content="summary_large_image" />
                <Meta name="twitter:image" :content="fileUrl" />
            </template>

            <template v-else-if="isVideo">
                <Meta property="og:type" content="video.other" />
                <Meta property="og:url" :content="fileUrl" />
                <Meta property="og:video" :content="fileUrl" />
                <Meta property="og:video:url" :content="fileUrl" />
                <Meta property="og:video:secure_url" :content="fileUrl" />
                <Meta property="og:video:type" :content="data.mimeType" />

                <Meta name="twitter:title" :content="data.fileName" />
                <Meta name="twitter:card" content="player" />
                <Meta name="twitter:player" :content="fileUrl" />
                <Meta name="twitter:player:stream" :content="fileUrl" />
                <Meta name="twitter:player:stream:content_type" :content="data.mimeType" />
            </template>

            <template v-else-if="isAudio">
                <Meta property="og:type" content="music.song" />
                <Meta property="og:url" :content="fileUrl" />
                <Meta property="og:audio" :content="fileUrl" />
                <Meta property="og:audio:secure_url" :content="fileUrl" />
                <Meta property="og:audio:type" :content="data.mimeType" />

                <Meta name="twitter:title" :content="data.fileName" />
                <Meta name="twitter:card" content="player" />
                <Meta name="twitter:player" :content="fileUrl" />
                <Meta name="twitter:player:stream" :content="fileUrl" />
                <Meta name="twitter:player:stream:content_type" :content="data.mimeType" />
                <Meta name="twitter:player:width" content="720" />
                <Meta name="twitter:player:height" content="480" />
            </template>

            <Meta v-else property="og:url" :content="fileUrl" />
        </Head>

        <PasswordGate
            v-if="!data.id"
            :disabled="isVerifying"
            :error="passwordError"
            @submit="handlePassword"
        />

        <template v-else>
            <ViewHeader
                :title="data.fileName!"
                :subtitle="`${data.size!.formatted} · ${dayjs(data.createdAt).format('MMM D, YYYY')}`"
                content-class="max-w-5xl"
            >
                <UiToggleGroup
                    v-if="isText"
                    type="single"
                    variant="outline"
                    size="sm"
                    :model-value="previewMode ? 'preview' : 'raw'"
                    @update:model-value="$event && (previewMode = $event === 'preview')"
                >
                    <UiToggleGroupItem value="preview" aria-label="Preview">
                        <IEye />
                    </UiToggleGroupItem>
                    <UiToggleGroupItem value="raw" aria-label="Raw">
                        <ICode />
                    </UiToggleGroupItem>
                </UiToggleGroup>
                <UiButton variant="outline" size="sm" @click="handleCopy">
                    <ICopy />
                    <span class="hidden sm:inline">Copy</span>
                </UiButton>
                <UiButton size="sm" as-child>
                    <a :href="downloadUrl" download>
                        <IDownload />
                        <span class="hidden sm:inline">Download</span>
                    </a>
                </UiButton>
            </ViewHeader>

            <main
                class="mx-auto flex w-full max-w-5xl flex-1 flex-col items-center justify-center p-4"
            >
                <img
                    v-if="isImage"
                    :src="fileUrl"
                    :alt="data.fileName"
                    class="max-h-[calc(100svh-6rem)] max-w-full rounded-2xl object-contain"
                />
                <video
                    v-else-if="isVideo"
                    controls
                    :src="fileUrl"
                    :poster="
                        data.thumbnailUrl
                            ? `${data.thumbnailUrl}${data.password ? `?password=${encodeURIComponent(String(data.password))}` : ''}`
                            : undefined
                    "
                    class="max-h-[calc(100svh-6rem)] max-w-full rounded-2xl"
                />
                <UiCard v-else-if="isAudio" class="w-full max-w-md">
                    <UiCardContent class="flex flex-col items-center gap-6">
                        <IMusic class="size-10 text-muted-foreground" />
                        <audio controls :src="fileUrl" class="w-full" />
                    </UiCardContent>
                </UiCard>
                <UiCard v-else-if="isText" class="w-full flex-1">
                    <UiCardContent>
                        <div v-if="fileContent === undefined" class="flex flex-col gap-3">
                            <TextBlock v-for="i in 6" :key="i" variant="muted" class="w-full">
                                <UiSkeleton
                                    class="h-lh rounded-full"
                                    :style="{ width: `${90 - ((i * 13) % 40)}%` }"
                                />
                            </TextBlock>
                        </div>
                        <MarkdownRenderer v-else-if="previewMode" :content="renderedContent" />
                        <pre
                            v-else
                            class="overflow-x-auto font-mono text-sm break-all whitespace-pre-wrap"
                            >{{ fileContent }}</pre>
                    </UiCardContent>
                </UiCard>
                <UiCard v-else class="w-full max-w-sm">
                    <UiCardContent>
                        <EmptyState
                            :icon="getFileIcon(data as FileData)"
                            title="No preview available"
                            description="This file type can't be previewed, but you can download it."
                        >
                            <UiButton as-child>
                                <a :href="downloadUrl" download>
                                    <IDownload />
                                    Download
                                </a>
                            </UiButton>
                        </EmptyState>
                    </UiCardContent>
                </UiCard>
            </main>
        </template>
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

const route = useRoute();
const { $toast } = useNuxtApp();
const { copy } = useClipboard({ legacy: true });

const id = (route.params as any).id as string;

const { data: _data, error } = await useFetch<FileData & { embed: IEmbed }>(
    `/api/files/${id}${route.query.password ? `?password=${encodeURIComponent(String(route.query.password))}` : ''}`,
    {
        headers: useRequestHeaders(['x-forwarded-for', 'host']),
    },
);

if (error.value?.status === 404) {
    throw createError({ status: 404, message: 'File not found' });
} else if (error.value?.status === 403) {
    throw createError({
        status: 403,
        message: 'You do not have permission to access this page',
    });
}

const data = ref({
    ..._data.value,
    password: (route.query.password as string) || null,
});

const fileContent = ref<string>();
const previewMode = ref(true);
const passwordError = ref<string>();
const isVerifying = ref(false);

const isImage = computed(() => data.value.mimeType?.startsWith('image/'));
const isVideo = computed(() => data.value.mimeType?.startsWith('video/'));
const isAudio = computed(() => data.value.mimeType?.startsWith('audio/'));
const extension = computed(() => getExtname(data.value.fileName || '').slice(1));
const isText = computed(() => TEXT_FILE_TYPES.some((type) => type.extension === extension.value));
const codeExtension = computed(
    () => TEXT_FILE_TYPES.slice(2).find((type) => type.extension === extension.value)?.extension,
);

const fileUrl = computed(
    () =>
        `${data.value.directUrl}${data.value.password ? `?password=${encodeURIComponent(String(data.value.password))}` : ''}`,
);

const downloadUrl = computed(() => `${fileUrl.value}${data.value.password ? '&' : '?'}download`);

const renderedContent = computed(() =>
    codeExtension.value
        ? `\`\`\`${codeExtension.value}\n${fileContent.value}\n\`\`\``
        : fileContent.value!,
);

const replaceString = (str: string) => {
    const dateFormat = 'MMMM D, YYYY [at] h:mm A';

    const params = {
        '{fileName}': data.value.fileName,
        '{mimeType}': data.value.mimeType,
        '{size}': data.value.size!.formatted,
        '{createdAt}': dayjs(data.value.createdAt).format(dateFormat),
        '{now}': dayjs().format(dateFormat),
    };

    return str.replace(
        /{fileName}|{mimeType}|{size}|{createdAt}|{now}/g,
        (matched) => params[matched as never],
    );
};

const handlePassword = async (password: string) => {
    passwordError.value = undefined;
    isVerifying.value = true;

    try {
        const file = await $fetch<FileData & { embed: IEmbed }>(`/api/files/${id}`, {
            query: { password },
        });

        data.value = { ...file, password };
    } catch (error: any) {
        passwordError.value = getErrorMessage(error);
    }

    isVerifying.value = false;
};

const handleCopy = () => {
    if (isText.value) {
        copy(fileContent.value || '');
        $toast.success('Content copied to clipboard');
    } else {
        copy(window.location.href);
        $toast.success('Link copied to clipboard');
    }
};

watch(
    () => data.value.id,
    async () => {
        if (!isText.value || !data.value.id) return;

        const file = await $fetch<Blob | string>(`/u/${id}`, {
            query: { password: data.value.password },
        });

        fileContent.value = file instanceof Blob ? await file.text() : file;
    },
    { immediate: true },
);
</script>
