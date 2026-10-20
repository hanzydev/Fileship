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
            </dl>

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
        </UiCardContent>
        <UiCardFooter>
            <UiButton
                variant="outline"
                :disabled="status.state === 'off'"
                @click="desktop('harbor:open')"
            >
                <IFolderOpen />
                {{ $t('harborSettings.openInExplorer') }}
            </UiButton>
        </UiCardFooter>
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
import { ICloud, ICloudDownload, IFilePen, IFolderSync, IFolderTree, ITrash2 } from '#components';

import type { HarborStatus } from '../../../shared/ipc';

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
        icon: IFolderTree,
        title: 'harborSettings.tips.folders',
        description: 'harborSettings.tips.foldersDescription',
    },
];

const settings = useSettings();
const { $toast } = useNuxtApp();

const status = ref<HarborStatus>({ state: 'off', root: '', files: 0, folders: 0, error: null });
const isBusy = ref(false);

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

onMounted(async () => {
    const off = window.fileship.on('harbor:status', (value) => {
        status.value = value;
    });

    onBeforeUnmount(off);

    status.value = await desktop('harbor:status');
});
</script>
