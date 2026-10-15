<template>
    <Head>
        <Title>Home</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader
            :title="`Welcome, ${account?.user.username}`"
            :description="`Connected to ${account?.siteName}.`"
        >
            <UiButton variant="outline" @click="handleRun('uploadFile')">
                <IUpload />
                Upload
            </UiButton>
            <UiButton @click="handleRun('captureRegion')">
                <IScanLine />
                Capture
            </UiButton>
        </PageHeader>

        <UiCard v-if="isLoaded && !items.length" class="flex-1">
            <UiCardContent class="flex flex-1">
                <EmptyState
                    :icon="IImage"
                    title="No captures yet"
                    :description="emptyDescription"
                    class="flex-1"
                />
            </UiCardContent>
        </UiCard>

        <template v-else>
            <div class="flex items-center justify-between gap-2">
                <TextBlock variant="muted">{{ pluralize(total, 'capture') }}</TextBlock>
                <UiButton v-if="items.length" variant="ghost" size="sm" @click="handleClear">
                    <ITrash2 />
                    Clear history
                </UiButton>
            </div>

            <VirtualList
                :items="items"
                :item-key="(item) => item.id"
                :min-item-width="240"
                :estimate-size="236"
                :gap="16"
                :loading="isLoading"
                @reach-end="handleLoadMore"
            >
                <template #default="{ item }">
                    <UiCard size="sm" class="group">
                        <UiCardContent class="flex flex-col gap-3">
                            <div
                                class="relative flex aspect-video items-center justify-center overflow-hidden rounded-2xl bg-muted"
                            >
                                <img
                                    v-if="item.thumbnail"
                                    :src="item.thumbnail"
                                    :alt="item.name"
                                    class="size-full object-cover"
                                />
                                <component
                                    :is="KIND_ICONS[item.kind]"
                                    v-else
                                    class="size-10 text-muted-foreground"
                                />

                                <div
                                    class="absolute top-2 right-2 opacity-0 transition-opacity group-hover:opacity-100 has-data-[state=open]:opacity-100"
                                >
                                    <UiDropdownMenu>
                                        <UiDropdownMenuTrigger as-child>
                                            <UiButton
                                                variant="secondary"
                                                size="icon-sm"
                                                aria-label="Actions"
                                            >
                                                <IEllipsis />
                                            </UiButton>
                                        </UiDropdownMenuTrigger>
                                        <UiDropdownMenuContent align="end">
                                            <template v-if="item.url">
                                                <UiDropdownMenuItem
                                                    @click="handleItem('history:copy', item)"
                                                >
                                                    <ICopy />
                                                    Copy link
                                                </UiDropdownMenuItem>
                                                <UiDropdownMenuItem
                                                    @click="handleItem('history:open', item)"
                                                >
                                                    <IExternalLink />
                                                    Open link
                                                </UiDropdownMenuItem>
                                            </template>
                                            <UiDropdownMenuItem
                                                v-if="item.localPath"
                                                @click="handleItem('history:showFile', item)"
                                            >
                                                <IFolderOpen />
                                                Show in folder
                                            </UiDropdownMenuItem>
                                            <UiDropdownMenuSeparator
                                                v-if="item.url || item.localPath"
                                            />
                                            <UiDropdownMenuItem
                                                variant="destructive"
                                                @click="handleItem('history:remove', item)"
                                            >
                                                <ITrash2 />
                                                Remove from history
                                            </UiDropdownMenuItem>
                                        </UiDropdownMenuContent>
                                    </UiDropdownMenu>
                                </div>
                            </div>

                            <div class="flex min-w-0 flex-col gap-1">
                                <TextBlock variant="small" class="truncate">
                                    {{ item.name }}
                                </TextBlock>

                                <UiProgress
                                    v-if="progress[item.id]"
                                    :model-value="progress[item.id]!.progress"
                                    class="my-1.5"
                                />
                                <TextBlock
                                    v-else
                                    variant="muted"
                                    :class="cn('truncate', item.error && 'text-destructive')"
                                >
                                    {{ describe(item) }}
                                </TextBlock>
                            </div>
                        </UiCardContent>
                    </UiCard>
                </template>

                <template #skeleton>
                    <UiSkeleton class="h-59 rounded-4xl" />
                </template>
            </VirtualList>
        </template>

        <div
            v-if="isOverDropZone"
            class="pointer-events-none fixed inset-0 z-50 flex items-center justify-center bg-background/80 backdrop-blur-sm"
        >
            <EmptyState
                :icon="IUpload"
                title="Drop to upload"
                description="Files go to Fileship with your upload settings."
            />
        </div>
    </div>
</template>

<script setup lang="ts">
import { filesize } from 'filesize';
import type { Component } from 'vue';

import { formatTimeAgo } from '@vueuse/core';

import { IFile, IFilm, IImage, IImagePlay, IUpload } from '#components';

import type { HistoryItem, HistoryKind, UploadProgress } from '../../shared/ipc';
import type { HotkeyAction } from '../../shared/settings';

const PAGE_SIZE = 60;

const KIND_ICONS: Record<HistoryKind, Component> = {
    screenshot: IImage,
    recording: IFilm,
    gif: IImagePlay,
    file: IFile,
};

const account = useAccount();
const { $toast } = useNuxtApp();

const items = ref<HistoryItem[]>([]);
const total = ref(0);
const isLoading = ref(false);
const isLoaded = ref(false);
const progress = ref<Record<string, UploadProgress>>({});
const settings = ref<{ hotkey: string } | null>(null);
const now = useNow({ scheduler: (callback) => useIntervalFn(callback, 30_000) });

const emptyDescription = computed(() =>
    settings.value?.hotkey
        ? `Press ${settings.value.hotkey.replaceAll('+', ' + ')} to capture a region, or drop files here to upload them.`
        : 'Capture a region from the tray icon, or drop files here to upload them.',
);

const describe = (item: HistoryItem) => {
    const time = formatTimeAgo(new Date(item.createdAt), {}, now.value);

    if (item.error) return item.error;
    if (item.shortUrl ?? item.url) return `${time} · ${filesize(item.size)}`;
    if (item.localPath) return `${time} · Saved on this computer`;
    return `${time} · Not uploaded`;
};

const load = async (offset: number, limit: number) => {
    const result = await desktop('history:list', offset, limit);
    total.value = result.total;
    return result.items;
};

const handleRefresh = async () => {
    items.value = await load(0, Math.max(items.value.length, PAGE_SIZE));
    isLoaded.value = true;
};

const handleLoadMore = async () => {
    if (isLoading.value || !isLoaded.value || items.value.length >= total.value) return;

    isLoading.value = true;

    try {
        items.value.push(...(await load(items.value.length, PAGE_SIZE)));
    } finally {
        isLoading.value = false;
    }
};

const handleRun = (action: HotkeyAction) => desktop('capture:run', action);

const handleItem = async (
    channel: 'history:copy' | 'history:open' | 'history:showFile' | 'history:remove',
    item: HistoryItem,
) => {
    try {
        await desktop(channel, item.id);
        if (channel === 'history:copy') $toast.success('Link copied');
    } catch (error: any) {
        $toast.error(error.message);
    }
};

const handleClear = () =>
    askConfirmation({
        title: 'Clear history?',
        description: 'Uploaded files stay on Fileship and saved files stay on this computer.',
        confirmText: 'Clear',
        destructive: true,
        action: () => desktop('history:clear'),
    });

const setProgress = (value: UploadProgress[]) => {
    progress.value = Object.fromEntries(value.map((item) => [item.id, item]));
};

const { isOverDropZone } = useDropZone(import.meta.client ? document.body : undefined, {
    onDrop: (files) => {
        const paths = (files ?? [])
            .map((file) => window.fileship.pathForFile(file))
            .filter(Boolean);
        if (paths.length) desktop('uploads:paths', paths);
    },
    checkValidity: (items) => [...items].some((item) => item.kind === 'file'),
    multiple: true,
});

onMounted(async () => {
    const offHistory = window.fileship.on('history:changed', handleRefresh);
    const offProgress = window.fileship.on('uploads:progress', setProgress);

    onBeforeUnmount(() => {
        offHistory();
        offProgress();
    });

    const [current, savedSettings] = await Promise.all([
        desktop('uploads:progress'),
        desktop('settings:get'),
        handleRefresh(),
    ]);

    setProgress(current);
    settings.value = {
        hotkey: savedSettings.hotkeysEnabled ? savedSettings.hotkeys.captureRegion : '',
    };
});

definePageMeta({
    layout: 'desktop',
});
</script>
