<template>
    <NuxtLoadingIndicator color="var(--primary)" :height="2" />
    <AppToaster />

    <NuxtLayout>
        <NuxtPage />
    </NuxtLayout>
</template>

<script setup lang="ts">
const { locale } = useI18n();

useHead({
    htmlAttrs: { lang: locale },
    titleTemplate: (title) => (title ? `${title} · Fileship` : 'Fileship'),
});
</script>
