<template>
    <NuxtLoadingIndicator
        color="linear-gradient(
            to right,
            color-mix(in srgb, var(--color-primary) 80%, white),
            var(--color-primary)
        )"
        :throttle="0"
    />
    <Toaster position="top-center" theme="dark" />

    <Head>
        <Title>{{ runtimeConfig.public.site.name }}</Title>
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

const runtimeConfig = useRuntimeConfig();
const currentUser = useAuthUser();

onMounted(initSocket);

watch(currentUser, (value) => {
    const adminSessionId = useCookie('adminSessionId');
    if (!adminSessionId.value) {
        if (!value) closeSocket();
        else if (!getSocket()) initSocket();
    }
});

useHead({
    titleTemplate: (title) =>
        title
            ? title === runtimeConfig.public.site.name || title.match(/\.|:/)
                ? title
                : `${title} · ${runtimeConfig.public.site.name}`
            : runtimeConfig.public.site.name,
});
</script>
