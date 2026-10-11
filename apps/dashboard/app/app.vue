<template>
    <NuxtLoadingIndicator color="var(--primary)" :height="2" />
    <UiSonner position="top-center" :theme="colorMode.value as never" />

    <Head>
        <Title>{{ publicSettings.siteName }}</Title>
    </Head>
    <NuxtLayout>
        <NuxtPage />
    </NuxtLayout>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';
import duration from 'dayjs/plugin/duration';
import relativeTime from 'dayjs/plugin/relativeTime';

dayjs.extend(duration);
dayjs.extend(relativeTime);

const publicSettings = usePublicSettings();
const currentUser = useAuthUser();
const currentTheme = useTheme();
const colorMode = useColorMode();

onMounted(initSocket);

watch(currentUser, (value) => {
    const adminSessionId = useCookie('adminSessionId');
    if (!adminSessionId.value) {
        if (!value) closeSocket();
        else if (!getSocket()) initSocket();
    }
});

useHead({
    htmlAttrs: {
        'data-accent': currentTheme,
    },
    titleTemplate: (title) =>
        title
            ? title === publicSettings.value.siteName || title.match(/\.|:/)
                ? title
                : `${title} · ${publicSettings.value.siteName}`
            : publicSettings.value.siteName,
});
</script>
