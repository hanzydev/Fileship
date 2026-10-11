<template>
    <Head>
        <Title>Files</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-6 p-4 pt-0">
        <PageHeader title="Files" :description="description" />
        <FileBrowser v-model:summary="summary" />
    </div>
</template>

<script setup lang="ts">
import { filesize } from 'filesize';

const summary = ref<FileSummary>();

const description = computed(() =>
    summary.value
        ? `${pluralize(summary.value.count, 'file')} · ${filesize(+summary.value.size)}`
        : 'Loading...',
);

definePageMeta({
    layout: 'dashboard',
    middleware: 'file-uploader-only',
});
</script>
