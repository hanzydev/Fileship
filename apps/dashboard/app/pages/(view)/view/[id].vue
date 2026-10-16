<template>
    <div class="flex min-h-svh flex-col">
        <Head>
            <Title v-if="!data.id">{{ $t('view.verifyPassword') }}</Title>
            <Title v-else>{{ data.name }}</Title>

            <template v-if="data?.embed?.enabled">
                <Meta name="theme-color" :content="data.embed.color" />
                <Meta
                    property="og:title"
                    :content="replaceString(data.embed.title || data.name!)"
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

                <Meta name="twitter:title" :content="data.name" />
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

                <Meta name="twitter:title" :content="data.name" />
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

                <Meta name="twitter:title" :content="data.name" />
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
                :title="data.name!"
                :subtitle="
                    $t('view.fileSubtitle', {
                        size: filesize(Number(data.size!.raw), { locale }),
                        date: date(data.createdAt).format('ll'),
                    })
                "
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
                    <UiToggleGroupItem value="preview" :aria-label="$t('view.preview')">
                        <IEye />
                    </UiToggleGroupItem>
                    <UiToggleGroupItem value="raw" :aria-label="$t('view.raw')">
                        <ICode />
                    </UiToggleGroupItem>
                </UiToggleGroup>
                <UiButton variant="outline" size="sm" @click="handleCopy">
                    <ICopy />
                    <span class="hidden sm:inline">{{ $t('view.copy') }}</span>
                </UiButton>
                <UiButton size="sm" as-child>
                    <a :href="downloadUrl" download>
                        <IDownload />
                        <span class="hidden sm:inline">{{ $t('view.download') }}</span>
                    </a>
                </UiButton>
            </ViewHeader>

            <main
                class="mx-auto flex w-full max-w-5xl flex-1 flex-col items-center justify-center p-4"
            >
                <img
                    v-if="isImage"
                    :src="fileUrl"
                    :alt="data.name"
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
                        <TextBlock v-if="contentError" variant="muted">
                            {{ contentError }}
                        </TextBlock>
                        <div v-else-if="fileContent === undefined" class="flex flex-col gap-3">
                            <TextBlock
                                v-for="i in 6"
                                :key="i"
                                custom-tag="div"
                                variant="muted"
                                class="w-full"
                            >
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
                            :title="$t('view.noPreview')"
                            :description="$t('view.noPreviewDescription')"
                        >
                            <UiButton as-child>
                                <a :href="downloadUrl" download>
                                    <IDownload />
                                    {{ $t('view.download') }}
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
import { filesize } from 'filesize';

import type { FileData } from '~/types/files';

const route = useRoute();
const { $toast } = useNuxtApp();
const { t, locale } = useI18n();
const date = useDate();
const { copy } = useClipboard({ legacy: true });

const id = (route.params as any).id as string;

const { data: _data, error } = await useFetch<FileData & { embed: IEmbed }>(
    `/api/files/${id}${route.query.password ? `?password=${encodeURIComponent(String(route.query.password))}` : ''}`,
    {
        headers: useRequestHeaders(['x-forwarded-for', 'host']),
    },
);

if (error.value?.status === 404) {
    throw createError({ status: 404, message: t('view.fileNotFound') });
} else if (error.value?.status === 403) {
    throw createError({
        status: 403,
        message: t('view.noPermission'),
    });
}

const data = ref({
    ..._data.value,
    password: (route.query.password as string) || null,
});

const fileContent = ref<string>();
const contentError = ref<string>();
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
    const params = {
        '{fileName}': data.value.fileName,
        '{mimeType}': data.value.mimeType,
        '{size}': data.value.size!.formatted,
        '{createdAt}': date(data.value.createdAt).format('LLL'),
        '{now}': date().format('LLL'),
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
        $toast.success(t('view.contentCopied'));
    } else {
        copy(window.location.href);
        $toast.success(t('view.linkCopied'));
    }
};

watch(
    () => data.value.id,
    async () => {
        if (import.meta.server || !isText.value || !data.value.id) return;

        try {
            const file = await $fetch<Blob | string>(`/u/${id}`, {
                query: { password: data.value.password },
            });

            contentError.value = undefined;
            fileContent.value = file instanceof Blob ? await file.text() : file;
        } catch (error) {
            contentError.value = getErrorMessage(error);
        }
    },
    { immediate: true },
);
</script>
