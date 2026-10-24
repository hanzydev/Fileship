<template>
    <div
        class="mx-auto flex min-h-svh w-full max-w-2xl flex-col gap-6 px-4 py-8"
        @dragover.prevent="isDragging = true"
        @dragleave.self="isDragging = false"
        @drop.prevent="handleDrop"
    >
        <header class="flex items-center gap-3">
            <img src="/icon/128.png" alt="Fileship" class="size-10" />
            <div class="flex flex-col gap-1">
                <TextBlock variant="h4">{{ $t('upload.title') }}</TextBlock>
                <TextBlock variant="muted">{{ $t('upload.description') }}</TextBlock>
            </div>
        </header>

        <UiCard v-if="settings && !settings.token">
            <UiCardContent>
                <EmptyState
                    :icon="PlugIcon"
                    :title="$t('popup.connectTitle')"
                    :description="$t('popup.connectDescription')"
                >
                    <UiButton @click="handleOpenSettings">{{ $t('popup.openSettings') }}</UiButton>
                </EmptyState>
            </UiCardContent>
        </UiCard>

        <template v-else-if="settings">
            <div
                :class="
                    cn(
                        'rounded-4xl border-2 border-dashed transition-colors',
                        isDragging && 'border-primary bg-primary/5',
                    )
                "
            >
                <EmptyState
                    :icon="UploadIcon"
                    :title="$t('upload.dropTitle')"
                    :description="$t('upload.dropDescription')"
                >
                    <UiButton @click="fileInput!.click()">
                        <IFolderOpen />
                        {{ $t('upload.choose') }}
                    </UiButton>
                </EmptyState>
            </div>

            <TaskList />
        </template>

        <input ref="fileInput" type="file" multiple hidden @change="handleChoose" />
    </div>
</template>

<script setup lang="ts">
import { PlugIcon, UploadIcon } from '@lucide/vue';

const settings = useSettings();
const isDragging = ref(false);
const fileInput = useTemplateRef<HTMLInputElement>('fileInput');

const queueFiles = async (files: File[]) => {
    if (files.length) await sendMessage({ type: 'uploadFiles', files: await stashFiles(files) });
};

const handleChoose = () => {
    const files = [...fileInput.value!.files!];
    fileInput.value!.value = '';
    queueFiles(files);
};

const handleDrop = (event: DragEvent) => {
    isDragging.value = false;
    if (settings.value?.token) queueFiles([...event.dataTransfer!.files]);
};

const handleOpenSettings = () => browser.runtime.openOptionsPage();
</script>
