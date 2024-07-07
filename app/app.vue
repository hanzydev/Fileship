<template>
    <NuxtLoadingIndicator color="var(--color-fs-accent)" />
    <Toaster position="top-center" theme="dark" />

    <Head>
        <Title>{{ appConfig.site.name }}</Title>
    </Head>
    <Body hfull wfull bg-fs5 text-white font-sans antialiased>
        <NuxtLayout>
            <NuxtPage />
        </NuxtLayout>
    </Body>
</template>

<script setup lang="ts">
import '~/styles/apexcharts.css';
import '~/styles/sonner.css';
import 'highlight.js/styles/tokyo-night-dark.css';

import { Toaster } from 'vue-sonner';

const appConfig = useAppConfig();
const currentTheme = useTheme();
const currentUser = useAuthUser();

onMounted(() => {
    currentTheme.value = window!.theme;
    initSocket();
});

watch(currentUser, (value) => {
    const adminSessionId = useCookie('adminSessionId');
    if (!adminSessionId.value) {
        if (!value) closeSocket();
        else if (!getSocket()) initSocket();
    }
});

watch(currentTheme, (value) => {
    if (window) window!.theme = value;
});

useHead({
    titleTemplate: (title) =>
        title
            ? title === appConfig.site?.name || title.match(/\.|:/)
                ? title
                : `${title} · ${appConfig.site?.name}`
            : appConfig.site?.name,
});
</script>
