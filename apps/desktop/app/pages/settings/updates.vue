<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>Updates</UiCardTitle>
            <UiCardDescription>
                Fileship {{ info?.version }}. Updates download in the background and install when
                you restart.
            </UiCardDescription>
            <UiCardAction>
                <UiButton v-if="updateStatus.state === 'ready'" size="sm" @click="handleInstall">
                    Restart now
                </UiButton>
                <UiButton
                    v-else
                    size="sm"
                    variant="outline"
                    :disabled="isWorking || updateStatus.state === 'unsupported'"
                    @click="handleCheck"
                >
                    <UiSpinner v-if="isWorking" />
                    <IRefreshCw v-else />
                    Check for updates
                </UiButton>
            </UiCardAction>
        </UiCardHeader>
        <UiCardContent class="flex flex-col gap-3">
            <TextBlock variant="muted">{{ statusText }}</TextBlock>
            <UiProgress
                v-if="updateStatus.state === 'downloading'"
                :model-value="updateStatus.progress ?? 0"
            />
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
import type { AppInfo } from '../../../shared/ipc';

const updateStatus = useUpdateStatus();

const info = ref<AppInfo | null>(null);

const isWorking = computed(() => ['checking', 'downloading'].includes(updateStatus.value.state));

const statusText = computed(() => {
    const { state, version, progress, error } = updateStatus.value;

    return {
        idle: 'Updates are checked when Fileship starts and every few hours.',
        checking: 'Checking for updates...',
        latest: 'You have the latest version.',
        downloading: `Downloading Fileship ${version}... ${progress ?? 0}%`,
        ready: `Fileship ${version} is ready to install.`,
        error: `The last check failed: ${error}. Fileship tries again later.`,
        unsupported: 'Updates are only available in the installed app.',
    }[state];
});

const handleCheck = () => desktop('updates:check');

const handleInstall = () => desktop('updates:install');

onMounted(async () => {
    info.value = await desktop('app:info');
});
</script>
