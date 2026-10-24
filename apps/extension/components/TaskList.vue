<template>
    <div class="flex flex-col gap-2">
        <div class="flex h-6 items-center justify-between gap-2">
            <TextBlock variant="small">{{ $t('tasks.title') }}</TextBlock>
            <UiButton v-if="tasks.length" variant="ghost" size="xs" @click="handleClear">
                {{ $t('tasks.clear') }}
            </UiButton>
        </div>

        <TextBlock v-if="!tasks.length" variant="muted">{{ $t('tasks.empty') }}</TextBlock>

        <UiItemGroup v-else class="gap-2">
            <UiItem v-for="task in tasks" :key="task.id" variant="outline" size="sm">
                <UiItemMedia variant="icon">
                    <component :is="KIND_ICONS[task.kind]" class="text-muted-foreground" />
                </UiItemMedia>
                <UiItemContent class="min-w-0 gap-1.5">
                    <UiItemTitle class="block w-full truncate">{{ task.name }}</UiItemTitle>
                    <UiProgress
                        v-if="task.status === 'running' && task.kind !== 'link'"
                        :model-value="task.progress"
                        class="h-1.5"
                    />
                    <UiItemDescription
                        :class="cn('truncate', task.status === 'failed' && 'text-destructive')"
                        :title="task.error ?? task.url"
                    >
                        {{ describe(task) }}
                    </UiItemDescription>
                </UiItemContent>
                <UiItemActions v-if="task.url">
                    <UiButton
                        variant="ghost"
                        size="icon-sm"
                        :aria-label="
                            copiedId === task.id ? $t('tasks.linkCopied') : $t('tasks.copyLink')
                        "
                        @click="handleCopy(task)"
                    >
                        <ICheck v-if="copiedId === task.id" />
                        <ICopy v-else />
                    </UiButton>
                    <UiButton
                        variant="ghost"
                        size="icon-sm"
                        as="a"
                        :href="task.url"
                        target="_blank"
                        rel="noreferrer"
                        :aria-label="$t('tasks.openLink')"
                    >
                        <IExternalLink />
                    </UiButton>
                </UiItemActions>
            </UiItem>
        </UiItemGroup>
    </div>
</template>

<script setup lang="ts">
import type { Component } from 'vue';

import { FileIcon, ImageIcon, LinkIcon, ScreenShareIcon } from '@lucide/vue';

const KIND_ICONS: Record<TaskKind, Component> = {
    file: FileIcon,
    image: ImageIcon,
    screenshot: ScreenShareIcon,
    link: LinkIcon,
};

const tasks = useTasks();
const copiedId = ref<string | null>(null);

let copiedTimer: ReturnType<typeof setTimeout> | undefined;

const describe = (task: Task) => {
    if (task.status === 'done') return task.url;
    if (task.status === 'failed') return task.error;
    if (task.kind === 'link') return t('tasks.shortening');

    return t('tasks.uploading', { progress: task.progress });
};

const handleCopy = async (task: Task) => {
    await navigator.clipboard.writeText(task.url!);

    copiedId.value = task.id;
    clearTimeout(copiedTimer);
    copiedTimer = setTimeout(() => (copiedId.value = null), 2000);
};

const handleClear = () => sendMessage({ type: 'clearTasks' });
</script>
