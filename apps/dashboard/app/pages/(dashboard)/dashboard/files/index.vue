<template>
    <Head>
        <Title>{{ $t('files.title') }}</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader :title="$t('files.title')" :description="description" />
        <FileBrowser v-model:summary="summary" />
    </div>
</template>

<script setup lang="ts">
import { filesize } from 'filesize';

import type { FileSummary } from '~/types/files';

const { t, locale } = useI18n();

const summary = ref<FileSummary>();

const description = computed(() =>
    summary.value
        ? t(
              'files.summary',
              {
                  count: summary.value.count,
                  size: filesize(+summary.value.size, { locale: locale.value }),
              },
              summary.value.count,
          )
        : t('common.loading'),
);

definePageMeta({
    layout: 'dashboard',
    middleware: 'file-uploader-only',
});
</script>
