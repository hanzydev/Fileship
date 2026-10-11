<template>
    <Head>
        <Title>Files</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-6 p-4 pt-0">
        <PageHeader title="Files" :description="description" />
        <FileBrowser
            :files="files"
            search-endpoint="/api/files/search"
            show-folder-filter
            :is-loading="isLoading"
        />
    </div>
</template>

<script setup lang="ts">
import { filesize } from 'filesize';

const files = useFiles();

const { status } = useLazyAsyncData('files', fetchFiles, { server: false });
useLazyAsyncData('folders', fetchFolders, { server: false });

const isLoading = computed(() => status.value !== 'success' && !files.value.length);

const description = computed(() => {
    const totalSize = files.value.reduce((total, file) => total + +file.size.raw, 0);
    return `${pluralize(files.value.length, 'file')} · ${filesize(totalSize)}`;
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'file-uploader-only',
});
</script>
