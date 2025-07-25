<template>
    <NuxtLoadingIndicator
        color="linear-gradient(
            to right,
            color-mix(in srgb, var(--fs-accent) 80%, white),
            var(--fs-accent)
        )"
        :throttle="0"
    />
    <Toaster position="top-center" theme="dark" />

    <Head>
        <Title>{{ appConfig.site.name }}</Title>
    </Head>
    <Body hfull wfull bg-fs-background text-white antialiased>
        <NuxtLayout>
            <NuxtPage />
        </NuxtLayout>
    </Body>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';
import duration from 'dayjs/plugin/duration';
import relativeTime from 'dayjs/plugin/relativeTime';

dayjs.extend(duration);
dayjs.extend(relativeTime);

const appConfig = useAppConfig();
const currentTheme = useTheme();
const currentUser = useAuthUser();

onMounted(() => {
    window!.theme = currentTheme.value;
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
