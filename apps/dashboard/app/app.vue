<template>
    <NuxtLoadingIndicator color="var(--primary)" :height="2" />
    <AppToaster />

    <Head>
        <Title>{{ publicSettings.siteName }}</Title>
    </Head>
    <NuxtLayout>
        <NuxtPage />
    </NuxtLayout>
</template>

<script setup lang="ts">
const publicSettings = usePublicSettings();
const currentUser = useAuthUser();
const currentTheme = useTheme();
const { locale } = useI18n();

onMounted(initSocket);

watch(currentUser, (value) => {
    if (!value) closeSocket();
    else if (!getSocket()) initSocket();
});

useHead({
    htmlAttrs: {
        lang: locale,
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
