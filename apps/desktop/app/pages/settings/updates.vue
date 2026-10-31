<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('updates.title') }}</UiCardTitle>
            <UiCardDescription>
                {{
                    $t(
                        appInfo.capabilities.updates === 'package'
                            ? 'updates.descriptionPackage'
                            : 'updates.description',
                        { version: appInfo.version },
                    )
                }}
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent class="flex flex-col gap-3">
            <TextBlock variant="muted">{{ statusText }}</TextBlock>
            <UiProgress
                v-if="updateStatus.state === 'downloading'"
                :model-value="updateStatus.progress ?? 0"
            />
        </UiCardContent>
        <UiCardFooter>
            <UiButton v-if="updateStatus.state === 'ready'" @click="handleInstall">
                {{ $t('updates.restartNow') }}
            </UiButton>
            <UiButton
                v-else
                variant="outline"
                :disabled="isWorking || updateStatus.state === 'unsupported'"
                @click="handleCheck"
            >
                <UiSpinner v-if="isWorking" />
                <IRefreshCw v-else />
                {{ $t('updates.check') }}
            </UiButton>
        </UiCardFooter>
    </UiCard>
</template>

<script setup lang="ts">
const appInfo = useAppInfo();
const updateStatus = useUpdateStatus();
const { t } = useI18n();

const isWorking = computed(() => ['checking', 'downloading'].includes(updateStatus.value.state));

const statusText = computed(() => {
    const { state, version, progress, error } = updateStatus.value;

    return t(`updates.status.${state}`, {
        version,
        progress: progress ?? 0,
        error: error?.replace(/\.$/, ''),
    });
});

const handleCheck = () => desktop('updates:check');

const handleInstall = () => desktop('updates:install');
</script>
