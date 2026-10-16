<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('backups.title') }}</UiCardTitle>
            <UiCardDescription>
                {{ $t('backups.description') }}
            </UiCardDescription>
            <UiCardAction class="flex gap-2">
                <UiButton
                    variant="outline"
                    :disabled="uploadProgress !== null"
                    @click="backupInput!.click()"
                >
                    <IUpload />
                    <span class="hidden sm:inline">{{ $t('backups.upload') }}</span>
                </UiButton>
                <UiButton :disabled="isCreating" @click="handleCreate">
                    <UiSpinner v-if="isCreating" />
                    <IPlus v-else />
                    <span class="hidden sm:inline">{{ $t('backups.create') }}</span>
                </UiButton>
            </UiCardAction>
        </UiCardHeader>
        <UiCardContent class="flex flex-col gap-3">
            <input
                ref="backupInput"
                type="file"
                accept=".tgz"
                class="sr-only"
                @change="handleUpload"
            />

            <UiCard v-if="uploadProgress !== null" size="sm">
                <UiCardContent class="flex flex-col gap-2">
                    <div class="flex items-center justify-between">
                        <TextBlock variant="small">{{ $t('backups.uploading') }}</TextBlock>
                        <TextBlock variant="muted">
                            {{ $t('backups.progress', { progress: uploadProgress }) }}
                        </TextBlock>
                    </div>
                    <UiProgress :model-value="uploadProgress" />
                </UiCardContent>
            </UiCard>

            <div v-if="isLoading" class="flex flex-col gap-2">
                <ListCardSkeleton v-for="i in 3" :key="i" media="size-5" />
            </div>

            <EmptyState
                v-else-if="isError"
                :icon="ICircleAlert"
                :title="$t('backups.loadFailed')"
                :description="$t('backups.loadFailedDescription')"
            >
                <UiButton variant="outline" @click="refresh">{{ $t('backups.tryAgain') }}</UiButton>
            </EmptyState>

            <EmptyState
                v-else-if="!backups.length"
                :icon="IArchive"
                :title="$t('backups.empty')"
                :description="$t('backups.emptyDescription')"
            />

            <VirtualList
                v-else
                :items="backups"
                :item-key="(b) => b.id"
                :estimate-size="66"
                :loading="isLoadingMore"
                @reach-end="loadMore"
            >
                <template #skeleton>
                    <ListCardSkeleton media="size-5" />
                </template>
                <template #default="{ item }">
                    <UiCard size="sm">
                        <UiCardContent class="flex min-h-8 items-center gap-3">
                            <IArchive class="size-5 shrink-0 text-muted-foreground" />
                            <div class="flex min-w-0 flex-1 flex-col">
                                <TextBlock variant="small">
                                    {{ date(item.createdAt).format('lll') }}
                                </TextBlock>
                                <TextBlock variant="muted" class="text-xs">
                                    {{ filesize(Number(item.size.raw), { locale }) }}
                                </TextBlock>
                            </div>
                            <UiButton variant="outline" size="sm" @click="handleRestore(item)">
                                <IHistory />
                                {{ $t('backups.restore') }}
                            </UiButton>
                            <UiDropdownMenu>
                                <UiDropdownMenuTrigger as-child>
                                    <UiButton variant="ghost" size="icon-sm">
                                        <IEllipsis />
                                    </UiButton>
                                </UiDropdownMenuTrigger>
                                <UiDropdownMenuContent align="end" class="w-56">
                                    <UiDropdownMenuItem
                                        @select="downloadFile(`/api/users/@me/backups/${item.id}`)"
                                    >
                                        <IDownload />
                                        {{ $t('backups.download') }}
                                    </UiDropdownMenuItem>
                                    <UiDropdownMenuItem
                                        variant="destructive"
                                        @select="handleDelete(item)"
                                    >
                                        <ITrash2 />
                                        {{ $t('backups.delete') }}
                                    </UiDropdownMenuItem>
                                </UiDropdownMenuContent>
                            </UiDropdownMenu>
                        </UiCardContent>
                    </UiCard>
                </template>
            </VirtualList>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
import { filesize } from 'filesize';
import { nanoid } from 'nanoid';

import { IArchive, ICircleAlert } from '#components';

import type { BackupData } from '~/types/backups';

const publicSettings = usePublicSettings();
const { $toast } = useNuxtApp();
const { t, locale } = useI18n();
const date = useDate();

const backupInput = useTemplateRef('backupInput');

const {
    items: backups,
    isLoading,
    isError,
    isLoadingMore,
    loadMore,
    refresh,
} = usePaginatedList<BackupData>('backups', '/api/users/@me/backups');

const isCreating = ref(false);
const uploadProgress = ref<number | null>(null);

const handleCreate = async () => {
    isCreating.value = true;

    try {
        await $fetch('/api/users/@me/backups', { method: 'POST' });
        $toast.success(t('backups.creating'));
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    isCreating.value = false;
};

const handleUpload = async (event: Event) => {
    const input = event.target as HTMLInputElement;
    const file = input.files![0]!;
    input.value = '';

    if (!file.name.endsWith('.tgz')) {
        $toast.error(t('backups.invalidFile'));
        return;
    }

    const chunkSize = publicSettings.value.fileChunkSize * 1024 * 1024;
    const chunks = Math.ceil(file.size / chunkSize);
    const uploadId = nanoid();

    uploadProgress.value = 0;

    try {
        for (let i = 0; i < chunks; i++) {
            const start = i * chunkSize;
            const end = Math.min(file.size, start + chunkSize);

            const formData = new FormData();
            formData.append(
                'backup',
                new Blob([file.slice(start, end)], { type: file.type }),
                file.name,
            );
            formData.append('currentChunk', (i + 1).toString());
            formData.append('totalChunks', chunks.toString());
            formData.append('chunkOffset', start.toString());
            formData.append('uploadId', uploadId);

            await $fetch('/api/users/@me/backups/upload', {
                method: 'POST',
                body: formData,
            });

            uploadProgress.value = Math.round((end / file.size) * 100);
        }

        $toast.success(t('backups.uploaded'));
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    uploadProgress.value = null;
};

const handleRestore = async (backup: BackupData) => {
    await askConfirmation({
        title: t('backups.restoreTitle'),
        description: t('backups.restoreDescription'),
        confirmText: t('backups.restore'),
        destructive: true,
        action: async () => {
            try {
                const result = await withVerification((verificationData) =>
                    $fetch(`/api/users/@me/backups/${backup.id}/load`, {
                        method: 'POST',
                        body: { verificationData },
                    }),
                );

                if (result) $toast.success(t('backups.restoring'));
            } catch (error: any) {
                $toast.error(getErrorMessage(error));
            }
        },
    });
};

const handleDelete = async (backup: BackupData) => {
    await askConfirmation({
        title: t('backups.deleteTitle'),
        description: t('backups.deleteDescription'),
        confirmText: t('backups.delete'),
        destructive: true,
        action: async () => {
            try {
                await $fetch(`/api/users/@me/backups/${backup.id}`, { method: 'DELETE' });
                $toast.success(t('backups.deleted'));
            } catch (error: any) {
                $toast.error(getErrorMessage(error));
            }
        },
    });
};
</script>
