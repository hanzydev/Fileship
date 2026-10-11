<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>Backups</UiCardTitle>
            <UiCardDescription>
                Export everything you have or restore a previous backup.
            </UiCardDescription>
            <UiCardAction class="flex gap-2">
                <UiButton
                    variant="outline"
                    :disabled="uploadProgress !== null"
                    @click="backupInput!.click()"
                >
                    <IUpload />
                    <span class="hidden sm:inline">Upload</span>
                </UiButton>
                <UiButton :disabled="isCreating" @click="handleCreate">
                    <UiSpinner v-if="isCreating" />
                    <IPlus v-else />
                    <span class="hidden sm:inline">Create</span>
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
                        <TextBlock variant="small">Uploading backup...</TextBlock>
                        <TextBlock variant="muted">{{ uploadProgress }}%</TextBlock>
                    </div>
                    <UiProgress :model-value="uploadProgress" />
                </UiCardContent>
            </UiCard>

            <div v-if="isLoading" class="flex flex-col gap-2">
                <ListCardSkeleton v-for="i in 3" :key="i" media="size-5" />
            </div>

            <EmptyState
                v-else-if="!backups.length"
                :icon="IArchive"
                title="No backups yet"
                description="Create one to export all of your files, folders and notes."
            />

            <VirtualList
                v-else
                :items="backups"
                :item-key="(b) => b.id"
                :estimate-size="66"
                :loading="isLoadingMore"
                @reach-end="loadMore"
            >
                <template #default="{ item }">
                    <UiCard size="sm">
                        <UiCardContent class="flex min-h-8 items-center gap-3">
                            <IArchive class="size-5 shrink-0 text-muted-foreground" />
                            <div class="flex min-w-0 flex-1 flex-col">
                                <TextBlock variant="small">
                                    {{ dayjs(item.createdAt).format('MMM D, YYYY HH:mm') }}
                                </TextBlock>
                                <TextBlock variant="muted" class="text-xs">
                                    {{ item.size.formatted }}
                                </TextBlock>
                            </div>
                            <UiButton variant="outline" size="sm" @click="handleRestore(item)">
                                <IHistory />
                                Restore
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
                                        Download
                                    </UiDropdownMenuItem>
                                    <UiDropdownMenuItem
                                        variant="destructive"
                                        @select="handleDelete(item)"
                                    >
                                        <ITrash2 />
                                        Delete
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
import dayjs from 'dayjs';

import { IArchive } from '#components';

const publicSettings = usePublicSettings();
const { $toast } = useNuxtApp();

const backupInput = useTemplateRef('backupInput');

const {
    items: backups,
    isLoading,
    isLoadingMore,
    loadMore,
} = usePaginatedList<BackupData>('backups', '/api/users/@me/backups');

const isCreating = ref(false);
const uploadProgress = ref<number | null>(null);

const handleCreate = async () => {
    isCreating.value = true;

    try {
        await $fetch('/api/users/@me/backups', { method: 'POST' });
        $toast.success('Backup is being created, this may take a while');
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
        $toast.error('Invalid backup file');
        return;
    }

    const chunkSize = publicSettings.value.fileChunkSize * 1024 * 1024;
    const chunks = Math.ceil(file.size / chunkSize);

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

            await $fetch('/api/users/@me/backups/upload', {
                method: 'POST',
                body: formData,
            });

            uploadProgress.value = Math.round((end / file.size) * 100);
        }

        $toast.success('Backup uploaded, you can now restore it');
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    uploadProgress.value = null;
};

const handleRestore = async (backup: BackupData) => {
    await askConfirmation({
        title: 'Restore this backup?',
        description: 'Your current data will be deleted and replaced with the backup data.',
        confirmText: 'Restore',
        destructive: true,
        action: async () => {
            try {
                const result = await withVerification((verificationData) =>
                    $fetch(`/api/users/@me/backups/${backup.id}/load`, {
                        method: 'POST',
                        body: { verificationData },
                    }),
                );

                if (result) $toast.success('Backup is being restored, this may take a while');
            } catch (error: any) {
                $toast.error(getErrorMessage(error));
            }
        },
    });
};

const handleDelete = async (backup: BackupData) => {
    await askConfirmation({
        title: 'Delete this backup?',
        description: 'This action cannot be undone.',
        confirmText: 'Delete',
        destructive: true,
        action: async () => {
            try {
                await $fetch(`/api/users/@me/backups/${backup.id}`, { method: 'DELETE' });
                $toast.success('Backup deleted');
            } catch (error: any) {
                $toast.error(getErrorMessage(error));
            }
        },
    });
};
</script>
