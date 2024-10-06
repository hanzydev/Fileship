<template>
    <div>
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
                    <Meta
                        property="og:site_name"
                        :content="replaceString(data.embed.siteName)"
                    />
                    <Meta
                        name="twitter:site"
                        :content="replaceString(data.embed.siteName)"
                    />
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
                <Meta
                    name="twitter:player:stream:content_type"
                    :content="data.mimeType"
                />
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
                <Meta
                    name="twitter:player:stream:content_type"
                    :content="data.mimeType"
                />
                <Meta name="twitter:player:width" content="720" />
                <Meta name="twitter:player:height" content="480" />
            </template>

            <Meta v-else property="og:url" :content="fileUrl" />
        </Head>

        <UiCentered
            h-screen="!"
            :class="(isVideo || isImage || isAudio) && '!p0'"
        >
            <VerifyPassword
                v-if="!data.id"
                :disabled="passwordDisabled"
                :error="passwordError"
                @password="handlePassword"
            />
            <template v-else>
                <img
                    v-if="isImage"
                    max-hfull
                    max-wfull
                    :src="fileUrl"
                    :alt="data.fileName"
                />
                <video
                    v-else-if="isVideo"
                    controls
                    max-hfull
                    max-wfull
                    :src="fileUrl"
                />
                <audio v-else-if="isAudio" controls :src="fileUrl" />
                <div
                    v-else
                    rounded-lg
                    bg-fs-overlay-1
                    p8
                    sm:max-w35rem
                    space-y-10
                >
                    <h2 line-clamp-2 break-all>{{ data.fileName }}</h2>
                    <p text-slate300 font-medium="!">
                        Sorry, we cannot preview this file. But you can download
                        it.
                    </p>
                    <UiButton
                        wfull
                        gap2
                        variant="accent"
                        alignment="center"
                        icon="heroicons-solid:download"
                        icon-size="20"
                        :href="`${fileUrl}${data.password ? '&' : '?'}download`"
                    >
                        Download
                    </UiButton>
                </div>
            </template>
        </UiCentered>
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

const route = useRoute();

const { data: _data, error } = await useFetch(
    `/api/files/${route.params.id}${route.query.password ? `?password=${route.query.password}` : ''}`,
    {
        headers: useRequestHeaders(['x-forwarded-for', 'host']),
    },
);

const data = ref({
    ..._data.value,
    password: route.query.password || null,
});

const isImage = computed(() => data.value?.mimeType?.startsWith('image/'));
const isVideo = computed(() => data.value?.mimeType?.startsWith('video/'));
const isAudio = computed(() => data.value?.mimeType?.startsWith('audio/'));

const fileUrl = computed(
    () =>
        `${data.value?.directUrl}${data.value?.password ? `?password=${data.value.password}` : ''}`,
);

if (error.value?.statusCode === 404) {
    throw createError({
        statusCode: 404,
        statusMessage: 'Not Found',
        message: 'File not found',
    });
} else if (error.value?.statusCode === 403) {
    throw createError({
        statusCode: 403,
        statusMessage: 'Forbidden',
        message: 'You do not have permission to access this page',
    });
}

const passwordError = ref<string>();
const passwordDisabled = ref<boolean>(false);

const replaceString = (str: string) => {
    const dateFormat = 'MMMM D, YYYY [at] h:mm A';

    const params = {
        '{fileName}': data.value!.fileName,
        '{mimeType}': data.value!.mimeType,
        '{size}': data.value!.size!.formatted,
        '{createdAt}': dayjs(data.value!.createdAt).format(dateFormat),
        '{now}': dayjs().format(dateFormat),
    };

    return str.replace(
        /{fileName}|{mimeType}|{size}|{createdAt}|{now}/g,
        (matched) => params[matched as never],
    );
};

const handlePassword = async (password: string) => {
    passwordError.value = undefined;
    passwordDisabled.value = true;

    try {
        const file = await $fetch(`/api/files/${route.params.id}`, {
            query: {
                password,
            },
        });

        data.value = {
            ...file,
            password,
        };
    } catch (error: any) {
        passwordError.value = error.data.message;
        passwordDisabled.value = false;
    }
};
</script>
