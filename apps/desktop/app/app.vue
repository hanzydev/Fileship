<template>
    <NuxtLoadingIndicator color="var(--primary)" :height="2" />
    <UiSonner position="top-center" :theme="colorMode.value as never" />

    <NuxtLayout>
        <NuxtPage />
    </NuxtLayout>
</template>

<script setup lang="ts">
const colorMode = useColorMode();

useHead({
    titleTemplate: (title) => (title ? `${title} · Fileship` : 'Fileship'),
});
</script>
