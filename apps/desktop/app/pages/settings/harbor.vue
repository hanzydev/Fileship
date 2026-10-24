<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('harborSettings.title') }}</UiCardTitle>
            <UiCardDescription>{{ $t('harborSettings.description') }}</UiCardDescription>
            <UiCardAction>
                <UiSwitch
                    :model-value="settings?.harbor.enabled"
                    :disabled="!settings || isBusy"
                    :aria-label="$t('harborSettings.enable')"
                    @update:model-value="handleToggle"
                />
            </UiCardAction>
        </UiCardHeader>
        <UiCardContent class="flex flex-col gap-4">
            <UiAlert v-if="status.error" variant="destructive">
                <ICircleAlert />
                <UiAlertTitle>{{ $t('harborSettings.problem') }}</UiAlertTitle>
                <UiAlertDescription>{{ status.error }}</UiAlertDescription>
            </UiAlert>

            <dl class="grid grid-cols-[auto_1fr] gap-x-6 gap-y-2 text-sm">
                <dt class="text-muted-foreground">{{ $t('harborSettings.status') }}</dt>
                <dd class="flex items-center justify-end gap-2">
                    <UiSpinner v-if="status.state === 'starting' || status.state === 'syncing'" />
                    {{ $t(`harborSettings.states.${status.state}`) }}
                </dd>
                <dt class="text-muted-foreground">{{ $t('harborSettings.content') }}</dt>
                <dd class="text-right">
                    {{
                        $t('harborSettings.contentSummary', {
                            files: $t('harborSettings.fileCount', status.files),
                            folders: $t('harborSettings.folderCount', status.folders),
                        })
                    }}
                </dd>
                <template v-if="status.lastSyncedAt">
                    <dt class="text-muted-foreground">{{ $t('harborSettings.lastSynced') }}</dt>
                    <dd class="text-right">{{ timeAgo(status.lastSyncedAt) }}</dd>
                </template>
                <template v-if="status.localSize !== null">
                    <dt class="text-muted-foreground">{{ $t('harborSettings.onThisDevice') }}</dt>
                    <dd class="text-right">{{ formatSize(status.localSize) }}</dd>
                </template>
                <template v-if="status.storage">
                    <dt class="text-muted-foreground">{{ $t('harborSettings.storage') }}</dt>
                    <dd class="flex items-center justify-end">
                        <i18n-t
                            v-if="status.storage.limit === null"
                            keypath="harborSettings.storageUnlimited"
                            tag="span"
                            class="inline-flex items-center gap-1"
                        >
                            <template #used>{{ formatSize(status.storage.used) }}</template>
                            <template #infinity>
                                <IInfinity
                                    class="size-4"
                                    :aria-label="$t('harborSettings.unlimited')"
                                />
                            </template>
                        </i18n-t>
                        <template v-else>
                            {{
                                $t('harborSettings.storageUsed', {
                                    used: formatSize(status.storage.used),
                                    limit: formatSize(status.storage.limit!),
                                    percent: percentFormat.format(storageRatio),
                                })
                            }}
                        </template>
                    </dd>
                    <UiProgress
                        v-if="status.storage.limit !== null"
                        :model-value="Math.min(storageRatio, 1) * 100"
                        :aria-label="$t('harborSettings.storage')"
                        :class="
                            cn(
                                'col-span-2 h-1.5',
                                storageRatio >= 0.9 &&
                                    '*:data-[slot=progress-indicator]:bg-destructive',
                            )
                        "
                    />
                </template>
            </dl>

            <div v-if="breakdown.length" class="flex flex-col gap-2">
                <div class="flex h-2 w-full overflow-hidden rounded-full bg-muted">
                    <div
                        v-for="item in breakdown"
                        :key="item.type"
                        class="h-full"
                        :style="{ width: `${item.share * 100}%`, background: item.color }"
                    />
                </div>
                <ul class="flex flex-wrap gap-x-4 gap-y-1 text-xs text-muted-foreground">
                    <li
                        v-for="item in breakdown"
                        :key="item.type"
                        class="flex items-center gap-1.5"
                    >
                        <span class="size-2 rounded-full" :style="{ background: item.color }" />
                        {{ $t(`harborSettings.categories.${item.type}`) }}
                        <span class="text-foreground tabular-nums">
                            {{ formatSize(item.size) }}
                        </span>
                    </li>
                </ul>
            </div>

            <UiField>
                <UiFieldLabel for="harbor-folder">{{ $t('harborSettings.location') }}</UiFieldLabel>
                <UiInputGroup>
                    <UiInputGroupInput
                        id="harbor-folder"
                        :model-value="settings?.harbor.folder || status.root"
                        :placeholder="$t('harborSettings.locationPlaceholder')"
                        readonly
                    />
                    <UiInputGroupAddon align="inline-end">
                        <UiInputGroupButton
                            v-if="settings?.harbor.folder"
                            :disabled="isBusy"
                            @click="handleFolder('')"
                        >
                            {{ $t('harborSettings.reset') }}
                        </UiInputGroupButton>
                        <UiInputGroupButton :disabled="isBusy" @click="handlePickFolder">
                            {{ $t('harborSettings.browse') }}
                        </UiInputGroupButton>
                    </UiInputGroupAddon>
                </UiInputGroup>
                <UiFieldDescription>
                    {{ $t('harborSettings.locationDescription') }}
                </UiFieldDescription>
            </UiField>

            <UiField orientation="horizontal" class="has-[>[data-slot=field-content]]:items-center">
                <UiFieldContent>
                    <UiFieldLabel for="harbor-copy-link">
                        {{ $t('harborSettings.copyLinkOnUpload') }}
                    </UiFieldLabel>
                    <UiFieldDescription>
                        {{ $t('harborSettings.copyLinkOnUploadDescription') }}
                    </UiFieldDescription>
                </UiFieldContent>
                <UiSwitch
                    id="harbor-copy-link"
                    :model-value="settings?.harbor.copyLinkOnUpload"
                    :disabled="!settings"
                    @update:model-value="updateSection('harbor', { copyLinkOnUpload: $event })"
                />
            </UiField>
        </UiCardContent>
        <UiCardFooter class="flex-wrap gap-2">
            <UiButton
                variant="outline"
                :disabled="status.state === 'off'"
                @click="desktop('harbor:open')"
            >
                <IFolderOpen />
                {{ $t('harborSettings.openInExplorer') }}
            </UiButton>
            <UiButton
                v-if="status.state === 'paused'"
                variant="outline"
                @click="handlePause(false)"
            >
                <IPlay />
                {{ $t('harbor.resume') }}
            </UiButton>
            <UiButton
                v-else
                variant="outline"
                :disabled="status.state !== 'ready' && status.state !== 'syncing'"
                @click="handlePause(true)"
            >
                <IPause />
                {{ $t('harbor.pause') }}
            </UiButton>
            <UiButton
                variant="outline"
                :disabled="!status.localSize || isFreeing || status.state === 'off'"
                @click="handleFreeUpSpace"
            >
                <UiSpinner v-if="isFreeing" />
                <IHardDriveDownload v-else />
                {{ $t('harbor.freeUpSpace') }}
            </UiButton>
        </UiCardFooter>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('harborSettings.activity.title') }}</UiCardTitle>
            <UiCardDescription>{{ $t('harborSettings.activity.description') }}</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <EmptyState
                v-if="!activity.length"
                :icon="IHistory"
                :title="$t('harborSettings.activity.emptyTitle')"
                :description="$t('harborSettings.activity.emptyDescription')"
            />
            <UiItemGroup v-else class="gap-2">
                <UiItem v-for="item in activity" :key="item.id" variant="muted" size="sm">
                    <UiItemMedia variant="icon">
                        <component
                            :is="ACTIVITY_ICONS[item.kind]"
                            :class="item.kind === 'error' && 'text-destructive'"
                        />
                    </UiItemMedia>
                    <UiItemContent class="min-w-0">
                        <UiItemTitle class="w-full truncate">{{ item.name }}</UiItemTitle>
                        <UiItemDescription class="truncate">
                            {{
                                $t('harborSettings.activity.meta', {
                                    what: item.detail ?? $t(`harborSettings.activity.${item.kind}`),
                                    time: timeAgo(item.at),
                                })
                            }}
                        </UiItemDescription>
                    </UiItemContent>
                </UiItem>
            </UiItemGroup>
        </UiCardContent>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('harborSettings.howItWorks') }}</UiCardTitle>
            <UiCardDescription>{{ $t('harborSettings.howItWorksDescription') }}</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiItemGroup class="gap-2">
                <UiItem v-for="item in TIPS" :key="item.title" variant="muted" size="sm">
                    <UiItemMedia variant="icon">
                        <component :is="item.icon" />
                    </UiItemMedia>
                    <UiItemContent>
                        <UiItemTitle>{{ $t(item.title) }}</UiItemTitle>
                        <UiItemDescription>{{ $t(item.description) }}</UiItemDescription>
                    </UiItemContent>
                </UiItem>
            </UiItemGroup>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
import type { Component } from 'vue';

import {
    ICircleAlert,
    ICloud,
    ICloudDownload,
    IFilePen,
    IFileX,
    IFolderInput,
    IFolderPlus,
    IFolderSync,
    IHardDriveDownload,
    IHistory,
    INotebookPen,
    IPause,
    IPencil,
    ITrash2,
    IUpload,
} from '#components';

import type {
    HarborActivity,
    HarborActivityKind,
    HarborBreakdown,
    HarborStatus,
} from '../../../shared/ipc';
import { formatStorageSize } from '../../../shared/size';
import { relativeTime } from '../../../shared/time';

const TIPS = [
    {
        icon: ICloud,
        title: 'harborSettings.tips.online',
        description: 'harborSettings.tips.onlineDescription',
    },
    {
        icon: ICloudDownload,
        title: 'harborSettings.tips.offline',
        description: 'harborSettings.tips.offlineDescription',
    },
    {
        icon: IFolderSync,
        title: 'harborSettings.tips.sync',
        description: 'harborSettings.tips.syncDescription',
    },
    {
        icon: IFilePen,
        title: 'harborSettings.tips.edits',
        description: 'harborSettings.tips.editsDescription',
    },
    {
        icon: ITrash2,
        title: 'harborSettings.tips.delete',
        description: 'harborSettings.tips.deleteDescription',
    },
    {
        icon: INotebookPen,
        title: 'harborSettings.tips.notes',
        description: 'harborSettings.tips.notesDescription',
    },
    {
        icon: IPause,
        title: 'harborSettings.tips.pause',
        description: 'harborSettings.tips.pauseDescription',
    },
];

const ACTIVITY_ICONS: Record<HarborActivityKind, Component> = {
    upload: IUpload,
    update: IFilePen,
    note: INotebookPen,
    folder: IFolderPlus,
    rename: IPencil,
    move: IFolderInput,
    trash: ITrash2,
    delete: IFileX,
    freeUp: IHardDriveDownload,
    error: ICircleAlert,
};

const settings = useSettings();
const { $toast } = useNuxtApp();
const { t, locale } = useI18n();
const now = useNow({ scheduler: (callback) => useIntervalFn(callback, 30_000) });

const status = ref<HarborStatus>({
    state: 'off',
    root: '',
    files: 0,
    folders: 0,
    error: null,
    storage: null,
    localSize: null,
    lastSyncedAt: null,
});
const activity = ref<HarborActivity[]>([]);
const sizes = ref<HarborBreakdown[]>([]);
const isBusy = ref(false);
const isFreeing = ref(false);

const percentFormat = computed(
    () => new Intl.NumberFormat(locale.value, { style: 'percent', maximumFractionDigits: 0 }),
);

const storageRatio = computed(() =>
    status.value.storage?.limit ? status.value.storage.used / status.value.storage.limit : 1,
);

const breakdown = computed(() => {
    const total = sizes.value.reduce((sum, item) => sum + item.size, 0);
    if (!total) return [];

    return sizes.value
        .filter((item) => item.size)
        .map((item, index) => ({
            ...item,
            share: item.size / total,
            color: `var(--chart-${(index % 5) + 1})`,
        }));
});

const formatSize = (bytes: number) => formatStorageSize(bytes, locale.value);

const timeAgo = (value: number) =>
    relativeTime(locale.value, value, now.value.getTime(), t('home.justNow'));

const handleToggle = async (enabled: boolean) => {
    if (!settings.value) return;
    await updateSettings({ harbor: { ...settings.value.harbor, enabled } });
};

const handleFolder = async (folder: string) => {
    isBusy.value = true;

    try {
        status.value = await desktop('harbor:setFolder', folder);
        settings.value = await desktop('settings:get');
    } catch (error: any) {
        $toast.error(error.message);
    } finally {
        isBusy.value = false;
    }
};

const handlePickFolder = async () => {
    const folder = await desktop('dialog:pickFolder');
    if (folder) await handleFolder(folder);
};

const handlePause = async (paused: boolean) => {
    status.value = await desktop('harbor:pause', paused);
};

const handleFreeUpSpace = async () => {
    isFreeing.value = true;

    try {
        const freed = await desktop('harbor:freeUpSpace');
        $toast.success(t('harbor.freedSpace', { size: formatSize(freed) }));
    } catch (error: any) {
        $toast.error(error.message);
    } finally {
        isFreeing.value = false;
    }
};

onMounted(async () => {
    const offStatus = window.fileship.on('harbor:status', (value) => {
        status.value = value;
    });
    const offActivity = window.fileship.on('harbor:activity', (value) => {
        activity.value = value;
    });

    onBeforeUnmount(() => {
        offStatus();
        offActivity();
    });

    [status.value, activity.value] = await Promise.all([
        desktop('harbor:status'),
        desktop('harbor:activity'),
    ]);

    if (status.value.state !== 'off') {
        sizes.value = await desktop('harbor:breakdown').catch(() => []);
    }
});
</script>
