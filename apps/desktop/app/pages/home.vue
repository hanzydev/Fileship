<template>
    <Head>
        <Title>{{ $t('nav.home') }}</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader
            :title="$t('home.welcome', { username: account?.user.username })"
            :description="$t('home.connectedTo', { siteName: account?.siteName })"
        >
            <UiButton variant="outline" @click="handleRun('uploadFile')">
                <IUpload />
                {{ $t('home.upload') }}
            </UiButton>
            <UiButton @click="handleRun('captureRegion')">
                <IScanLine />
                {{ $t('home.capture') }}
            </UiButton>
        </PageHeader>

        <UiCard v-if="isLoaded && !items.length" class="flex-1">
            <UiCardContent class="flex flex-1">
                <EmptyState
                    :icon="IImage"
                    :title="$t('home.emptyTitle')"
                    :description="emptyDescription"
                    class="flex-1"
                />
            </UiCardContent>
        </UiCard>

        <template v-else>
            <div class="flex items-center justify-between gap-2">
                <TextBlock variant="muted">{{ $t('home.captureCount', total) }}</TextBlock>
                <UiButton v-if="items.length" variant="ghost" size="sm" @click="handleClear">
                    <ITrash2 />
                    {{ $t('home.clearHistory') }}
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
                                    class="absolute top-2 right-2 opacity-0 transition-opacity group-hover:opacity-100 has-data-[state=open]:opacity-100 pointer-coarse:opacity-100"
                                >
                                    <UiDropdownMenu>
                                        <UiDropdownMenuTrigger as-child>
                                            <UiButton
                                                variant="secondary"
                                                size="icon-sm"
                                                :aria-label="$t('home.actions')"
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
                                                    {{ $t('home.copyLink') }}
                                                </UiDropdownMenuItem>
                                                <UiDropdownMenuItem
                                                    @click="handleItem('history:open', item)"
                                                >
                                                    <IExternalLink />
                                                    {{ $t('home.openLink') }}
                                                </UiDropdownMenuItem>
                                            </template>
                                            <UiDropdownMenuItem
                                                v-if="item.localPath"
                                                @click="handleItem('history:showFile', item)"
                                            >
                                                <IFolderOpen />
                                                {{ $t('home.showInFolder') }}
                                            </UiDropdownMenuItem>
                                            <UiDropdownMenuItem
                                                v-if="harborState === 'ready' && item.fileId"
                                                @click="handleItem('history:showInHarbor', item)"
                                            >
                                                <IAnchor />
                                                {{ $t('home.showInHarbor') }}
                                            </UiDropdownMenuItem>
                                            <UiDropdownMenuSeparator
                                                v-if="item.url || item.localPath"
                                            />
                                            <UiDropdownMenuItem
                                                variant="destructive"
                                                @click="handleItem('history:remove', item)"
                                            >
                                                <ITrash2 />
                                                {{ $t('home.removeFromHistory') }}
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

        <Transition
            enter-active-class="animate-in fade-in-0 duration-100"
            leave-active-class="animate-out fade-out-0 duration-100"
        >
            <div
                v-if="isOverDropZone"
                class="pointer-events-none fixed inset-0 z-50 flex items-center justify-center bg-background/80 backdrop-blur-sm"
            >
                <EmptyState
                    :icon="IUpload"
                    :title="$t('home.dropTitle')"
                    :description="$t('home.dropDescription')"
                />
            </div>
        </Transition>
    </div>
</template>

<script setup lang="ts">
import { filesize } from 'filesize';
import type { Component } from 'vue';

import { IFile, IFilm, IImage, IImagePlay, ILink, IUpload } from '#components';

import type { HarborStatus, HistoryItem, HistoryKind, UploadProgress } from '../../shared/ipc';
import type { HotkeyAction } from '../../shared/settings';

const PAGE_SIZE = 60;

const TIME_UNITS: [Intl.RelativeTimeFormatUnit, number][] = [
    ['year', 31_536_000],
    ['month', 2_592_000],
    ['week', 604_800],
    ['day', 86_400],
    ['hour', 3600],
    ['minute', 60],
];

const KIND_ICONS: Record<HistoryKind, Component> = {
    screenshot: IImage,
    recording: IFilm,
    gif: IImagePlay,
    file: IFile,
    link: ILink,
};

const account = useAccount();
const { $toast } = useNuxtApp();
const { t, locale } = useI18n();

const items = ref<HistoryItem[]>([]);
const total = ref(0);
const isLoading = ref(false);
const isLoaded = ref(false);
const progress = ref<Record<string, UploadProgress>>({});
const settings = ref<{ hotkey: string } | null>(null);
const harborState = ref<HarborStatus['state']>('off');
const now = useNow({ scheduler: (callback) => useIntervalFn(callback, 30_000) });

const relativeTime = computed(() => new Intl.RelativeTimeFormat(locale.value, { numeric: 'auto' }));

const emptyDescription = computed(() =>
    settings.value?.hotkey
        ? t('home.emptyWithHotkey', { hotkey: settings.value.hotkey.replaceAll('+', ' + ') })
        : t('home.emptyWithTray'),
);

const timeAgo = (value: number) => {
    const seconds = (value - now.value.getTime()) / 1000;
    const unit = TIME_UNITS.find(([, size]) => Math.abs(seconds) >= size);

    if (!unit) return t('home.justNow');
    return relativeTime.value.format(Math.round(seconds / unit[1]), unit[0]);
};

const describe = (item: HistoryItem) => {
    const time = timeAgo(item.createdAt);

    if (item.error) return item.error;
    if (item.kind === 'link') return t('home.shortenedMeta', { time });
    if (item.shortUrl ?? item.url)
        return t('home.uploadedMeta', {
            time,
            size: filesize(item.size, { locale: locale.value }),
        });
    if (item.localPath) return t('home.savedMeta', { time });
    return t('home.notUploadedMeta', { time });
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
    channel:
        | 'history:copy'
        | 'history:open'
        | 'history:showFile'
        | 'history:showInHarbor'
        | 'history:remove',
    item: HistoryItem,
) => {
    try {
        await desktop(channel, item.id);
        if (channel === 'history:copy') $toast.success(t('home.linkCopied'));
    } catch (error: any) {
        $toast.error(error.message);
    }
};

const handleClear = () =>
    askConfirmation({
        title: t('home.clearTitle'),
        description: t('home.clearDescription'),
        confirmText: t('home.clear'),
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
    const offHarbor = window.fileship.on('harbor:status', (value) => {
        harborState.value = value.state;
    });

    onBeforeUnmount(() => {
        offHistory();
        offProgress();
        offHarbor();
    });

    const [current, savedSettings, harbor] = await Promise.all([
        desktop('uploads:progress'),
        desktop('settings:get'),
        desktop('harbor:status'),
        handleRefresh(),
    ]);

    setProgress(current);
    harborState.value = harbor.state;
    settings.value = {
        hotkey: savedSettings.hotkeysEnabled ? savedSettings.hotkeys.captureRegion : '',
    };
});

definePageMeta({
    layout: 'desktop',
});
</script>
