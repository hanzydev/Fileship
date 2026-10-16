<template>
    <NuxtLoadingIndicator color="var(--primary)" :height="2" />
    <AppToaster />

    <NuxtLayout>
        <NuxtPage />
    </NuxtLayout>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';
import duration from 'dayjs/plugin/duration';
import localizedFormat from 'dayjs/plugin/localizedFormat';
import relativeTime from 'dayjs/plugin/relativeTime';

dayjs.extend(duration);
dayjs.extend(localizedFormat);
dayjs.extend(relativeTime);

useHead({
    titleTemplate: (title) => (title ? `${title} · Fileship Telemetry` : 'Fileship Telemetry'),
});
</script>
