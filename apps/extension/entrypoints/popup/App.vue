<template>
    <div class="flex w-90 flex-col gap-4 p-4">
        <header class="flex items-center gap-3">
            <img src="/icon/128.png" alt="Fileship" class="size-8" />
            <div class="flex min-w-0 flex-1 flex-col gap-1">
                <TextBlock variant="small" class="truncate">
                    {{ settings?.connection?.siteName ?? 'Fileship' }}
                </TextBlock>
                <TextBlock v-if="host" variant="muted" class="truncate text-xs">
                    {{ host }}
                </TextBlock>
            </div>
            <UiButton
                variant="ghost"
                size="icon-sm"
                :aria-label="$t('popup.settings')"
                @click="handleOpenSettings"
            >
                <ISettings />
            </UiButton>
        </header>

        <EmptyState
            v-if="settings && !host"
            :icon="PlugIcon"
            :title="$t('popup.connectTitle')"
            :description="$t('popup.connectDescription')"
            class="p-6"
        >
            <UiButton @click="handleOpenSettings">{{ $t('popup.openSettings') }}</UiButton>
        </EmptyState>

        <template v-else-if="settings">
            <UiItemGroup class="gap-1">
                <UiItem
                    v-for="action in actions"
                    :key="action.title"
                    as="button"
                    size="sm"
                    class="text-left hover:bg-muted disabled:pointer-events-none disabled:opacity-50"
                    :disabled="!action.isEnabled"
                    @click="action.run"
                >
                    <UiItemMedia variant="icon">
                        <component :is="action.icon" />
                    </UiItemMedia>
                    <UiItemContent>
                        <UiItemTitle>{{ action.title }}</UiItemTitle>
                        <UiItemDescription>{{ action.description }}</UiItemDescription>
                    </UiItemContent>
                </UiItem>
            </UiItemGroup>

            <UiAlert v-if="error" variant="destructive">
                <ICircleAlert />
                <UiAlertDescription>{{ error }}</UiAlertDescription>
            </UiAlert>

            <UiSeparator />

            <TaskList />
        </template>

        <input ref="fileInput" type="file" multiple hidden @change="handleFiles" />
    </div>
</template>

<script setup lang="ts">
import { LinkIcon, PlugIcon, ScanIcon, ScreenShareIcon, UploadIcon } from '@lucide/vue';

const settings = useSettings();
const tab = ref<Browser.tabs.Tab | null>(null);
const error = ref('');
const fileInput = useTemplateRef<HTMLInputElement>('fileInput');

browser.tabs
    .query({ active: true, currentWindow: true })
    .then(([activeTab]) => (tab.value = activeTab ?? null));

const host = computed(() =>
    settings.value?.instanceUrl && settings.value.token
        ? new URL(settings.value.instanceUrl).host
        : '',
);

const isWebPage = computed(() => /^https?:\/\//i.test(tab.value?.url ?? ''));

const actions = computed(() => {
    const canUpload = settings.value?.connection?.canUpload !== false;
    const canShorten = settings.value?.connection?.canShorten !== false;

    return [
        {
            icon: UploadIcon,
            title: t('popup.uploadFiles'),
            description: t('popup.uploadFilesDescription'),
            isEnabled: canUpload,
            run: handleUploadFiles,
        },
        {
            icon: LinkIcon,
            title: t('popup.shortenPage'),
            description: t('popup.shortenPageDescription'),
            isEnabled: canShorten && isWebPage.value,
            run: handleShortenPage,
        },
        {
            icon: ScreenShareIcon,
            title: t('popup.captureVisible'),
            description: t('popup.captureVisibleDescription'),
            isEnabled: canUpload && !!tab.value,
            run: handleCaptureVisible,
        },
        {
            icon: ScanIcon,
            title: t('popup.captureArea'),
            description: t('popup.captureAreaDescription'),
            isEnabled: canUpload && isWebPage.value,
            run: handleCaptureArea,
        },
    ];
});

const handleOpenSettings = () => {
    browser.runtime.openOptionsPage();
    window.close();
};

const handleUploadFiles = () => {
    if (!import.meta.env.FIREFOX) return fileInput.value!.click();

    browser.tabs.create({ url: browser.runtime.getURL('/upload.html') });
    window.close();
};

const handleFiles = async () => {
    const files = [...fileInput.value!.files!];
    fileInput.value!.value = '';

    if (files.length) await sendMessage({ type: 'uploadFiles', files: await stashFiles(files) });
};

const handleShortenPage = () => sendMessage({ type: 'shortenPage', url: tab.value!.url! });

const handleCaptureVisible = () =>
    sendMessage({ type: 'captureVisible', windowId: tab.value!.windowId });

const handleCaptureArea = async () => {
    const response = await sendMessage<{ error?: string }>({
        type: 'captureArea',
        tabId: tab.value!.id!,
    });

    if (response?.error) error.value = response.error;
    else window.close();
};
</script>
