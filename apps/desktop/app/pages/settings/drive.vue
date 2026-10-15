<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>Fileship drive</UiCardTitle>
            <UiCardDescription>
                Your files and folders in File Explorer. Files download when you open them.
            </UiCardDescription>
            <UiCardAction>
                <UiSwitch
                    :model-value="settings?.drive.enabled"
                    :disabled="!settings || isBusy"
                    aria-label="Enable the Fileship drive"
                    @update:model-value="handleToggle"
                />
            </UiCardAction>
        </UiCardHeader>
        <UiCardContent class="flex flex-col gap-4">
            <UiAlert v-if="status.error" variant="destructive">
                <ICircleAlert />
                <UiAlertTitle>The drive has a problem</UiAlertTitle>
                <UiAlertDescription>{{ status.error }}</UiAlertDescription>
            </UiAlert>

            <dl class="grid grid-cols-[auto_1fr] gap-x-6 gap-y-2 text-sm">
                <dt class="text-muted-foreground">Status</dt>
                <dd class="flex items-center justify-end gap-2">
                    <UiSpinner v-if="status.state === 'starting' || status.state === 'syncing'" />
                    {{ STATE_LABELS[status.state] }}
                </dd>
                <dt class="text-muted-foreground">Content</dt>
                <dd class="text-right">
                    {{ pluralize(status.files, 'file') }} in
                    {{ pluralize(status.folders, 'folder') }}
                </dd>
            </dl>

            <UiField>
                <UiFieldLabel for="drive-folder">Location</UiFieldLabel>
                <UiInputGroup>
                    <UiInputGroupInput
                        id="drive-folder"
                        :model-value="settings?.drive.folder || status.root"
                        placeholder="C:\Users\you\Fileship"
                        readonly
                    />
                    <UiInputGroupAddon align="inline-end">
                        <UiInputGroupButton
                            v-if="settings?.drive.folder"
                            :disabled="isBusy"
                            @click="handleFolder('')"
                        >
                            Reset
                        </UiInputGroupButton>
                        <UiInputGroupButton :disabled="isBusy" @click="handlePickFolder">
                            Browse
                        </UiInputGroupButton>
                    </UiInputGroupAddon>
                </UiInputGroup>
                <UiFieldDescription>
                    Moving the drive takes your synced files out of the old folder.
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
                Open in File Explorer
            </UiButton>
        </UiCardFooter>
    </UiCard>

    <UiCard v-if="uploads.length">
        <UiCardHeader>
            <UiCardTitle>Uploading</UiCardTitle>
            <UiCardDescription>
                {{ pluralize(uploads.length, 'file') }} from the drive
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent class="flex flex-col gap-4">
            <div v-for="upload in uploads" :key="upload.id" class="flex flex-col gap-2">
                <div class="flex items-center justify-between gap-4 text-sm">
                    <span class="truncate">{{ upload.name }}</span>
                    <span class="text-muted-foreground tabular-nums">{{ upload.progress }}%</span>
                </div>
                <UiProgress :model-value="upload.progress" />
            </div>
        </UiCardContent>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>How it works</UiCardTitle>
        </UiCardHeader>
        <UiCardContent>
            <UiItemGroup class="gap-2">
                <UiItem v-for="item in TIPS" :key="item.title" variant="muted" size="sm">
                    <UiItemMedia variant="icon">
                        <component :is="item.icon" />
                    </UiItemMedia>
                    <UiItemContent>
                        <UiItemTitle>{{ item.title }}</UiItemTitle>
                        <UiItemDescription>{{ item.description }}</UiItemDescription>
                    </UiItemContent>
                </UiItem>
            </UiItemGroup>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
import { ICloud, ICloudDownload, IFilePen, ITrash2 } from '#components';

import type { DriveStatus, UploadProgress } from '../../../shared/ipc';

const STATE_LABELS: Record<DriveStatus['state'], string> = {
    off: 'Off',
    starting: 'Starting',
    syncing: 'Syncing',
    ready: 'Up to date',
    error: 'Needs attention',
};

const TIPS = [
    {
        icon: ICloud,
        title: 'Files stay online',
        description: 'They take no space until you open them.',
    },
    {
        icon: ICloudDownload,
        title: 'Keep files offline',
        description:
            'Right click and choose "Always keep on this device", or "Free up space" to download them again later.',
    },
    {
        icon: IFilePen,
        title: 'Edits upload a new file',
        description: 'Saving a changed file uploads it again, so its link changes.',
    },
    {
        icon: ITrash2,
        title: 'Deleting moves to the trash',
        description: 'Files you delete in File Explorer go to the Fileship trash.',
    },
];

const settings = useSettings();
const { $toast } = useNuxtApp();

const status = ref<DriveStatus>({ state: 'off', root: '', files: 0, folders: 0, error: null });
const isBusy = ref(false);
const uploads = ref<UploadProgress[]>([]);

const setUploads = (value: UploadProgress[]) => {
    uploads.value = value.filter((item) => item.origin === 'drive');
};

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
    const offStatus = window.fileship.on('drive:status', (value) => {
        status.value = value;
    });
    const offUploads = window.fileship.on('uploads:progress', setUploads);

    onBeforeUnmount(() => {
        offStatus();
        offUploads();
    });

    const [current, progress] = await Promise.all([
        desktop('drive:status'),
        desktop('uploads:progress'),
    ]);

    status.value = current;
    setUploads(progress);
});
</script>
