<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('driveSettings.title') }}</UiCardTitle>
            <UiCardDescription>{{ $t('driveSettings.description') }}</UiCardDescription>
            <UiCardAction>
                <UiSwitch
                    :model-value="settings?.drive.enabled"
                    :disabled="!settings || isBusy"
                    :aria-label="$t('driveSettings.enable')"
                    @update:model-value="handleToggle"
                />
            </UiCardAction>
        </UiCardHeader>
        <UiCardContent class="flex flex-col gap-4">
            <UiAlert v-if="status.error" variant="destructive">
                <ICircleAlert />
                <UiAlertTitle>{{ $t('driveSettings.problem') }}</UiAlertTitle>
                <UiAlertDescription>{{ status.error }}</UiAlertDescription>
            </UiAlert>

            <dl class="grid grid-cols-[auto_1fr] gap-x-6 gap-y-2 text-sm">
                <dt class="text-muted-foreground">{{ $t('driveSettings.status') }}</dt>
                <dd class="flex items-center justify-end gap-2">
                    <UiSpinner v-if="status.state === 'starting' || status.state === 'syncing'" />
                    {{ $t(`driveSettings.states.${status.state}`) }}
                </dd>
                <dt class="text-muted-foreground">{{ $t('driveSettings.content') }}</dt>
                <dd class="text-right">
                    {{
                        $t('driveSettings.contentSummary', {
                            files: $t('driveSettings.fileCount', status.files),
                            folders: $t('driveSettings.folderCount', status.folders),
                        })
                    }}
                </dd>
            </dl>

            <UiField>
                <UiFieldLabel for="drive-folder">{{ $t('driveSettings.location') }}</UiFieldLabel>
                <UiInputGroup>
                    <UiInputGroupInput
                        id="drive-folder"
                        :model-value="settings?.drive.folder || status.root"
                        :placeholder="$t('driveSettings.locationPlaceholder')"
                        readonly
                    />
                    <UiInputGroupAddon align="inline-end">
                        <UiInputGroupButton
                            v-if="settings?.drive.folder"
                            :disabled="isBusy"
                            @click="handleFolder('')"
                        >
                            {{ $t('driveSettings.reset') }}
                        </UiInputGroupButton>
                        <UiInputGroupButton :disabled="isBusy" @click="handlePickFolder">
                            {{ $t('driveSettings.browse') }}
                        </UiInputGroupButton>
                    </UiInputGroupAddon>
                </UiInputGroup>
                <UiFieldDescription>
                    {{ $t('driveSettings.locationDescription') }}
                </UiFieldDescription>
            </UiField>
        </UiCardContent>
        <UiCardFooter>
            <UiButton
                variant="outline"
                :disabled="status.state === 'off'"
                @click="desktop('drive:open')"
            >
                <IFolderOpen />
                {{ $t('driveSettings.openInExplorer') }}
            </UiButton>
        </UiCardFooter>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('driveSettings.howItWorks') }}</UiCardTitle>
            <UiCardDescription>{{ $t('driveSettings.howItWorksDescription') }}</UiCardDescription>
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
import { ICloud, ICloudDownload, IFilePen, IFolderSync, IFolderTree, ITrash2 } from '#components';

import type { DriveStatus } from '../../../shared/ipc';

const TIPS = [
    {
        icon: ICloud,
        title: 'driveSettings.tips.online',
        description: 'driveSettings.tips.onlineDescription',
    },
    {
        icon: ICloudDownload,
        title: 'driveSettings.tips.offline',
        description: 'driveSettings.tips.offlineDescription',
    },
    {
        icon: IFolderSync,
        title: 'driveSettings.tips.sync',
        description: 'driveSettings.tips.syncDescription',
    },
    {
        icon: IFilePen,
        title: 'driveSettings.tips.edits',
        description: 'driveSettings.tips.editsDescription',
    },
    {
        icon: ITrash2,
        title: 'driveSettings.tips.delete',
        description: 'driveSettings.tips.deleteDescription',
    },
    {
        icon: IFolderTree,
        title: 'driveSettings.tips.folders',
        description: 'driveSettings.tips.foldersDescription',
    },
];

const settings = useSettings();
const { $toast } = useNuxtApp();

const status = ref<DriveStatus>({ state: 'off', root: '', files: 0, folders: 0, error: null });
const isBusy = ref(false);

const handleToggle = async (enabled: boolean) => {
    if (!settings.value) return;
    await updateSettings({ drive: { ...settings.value.drive, enabled } });
};

const handleFolder = async (folder: string) => {
    isBusy.value = true;

    try {
        status.value = await desktop('drive:setFolder', folder);
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

onMounted(async () => {
    const off = window.fileship.on('drive:status', (value) => {
        status.value = value;
    });

    onBeforeUnmount(off);

    status.value = await desktop('drive:status');
});
</script>
