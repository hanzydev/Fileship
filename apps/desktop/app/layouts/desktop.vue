<template>
    <main>
        <UiSidebarProvider storage-key="sidebar">
            <AppSidebar />
            <UiSidebarInset>
                <AppHeader :labels="breadcrumbLabels" />

                <UiAlert v-if="updateStatus.state === 'ready'" class="mx-4 mb-4 w-auto">
                    <IDownload />
                    <UiAlertTitle>
                        {{ $t('nav.updateReady', { version: updateStatus.version }) }}
                    </UiAlertTitle>
                    <UiAlertDescription>
                        {{ $t('nav.updateReadyDescription') }}
                    </UiAlertDescription>
                    <UiAlertAction>
                        <UiButton size="sm" @click="handleInstall">
                            {{ $t('nav.restartNow') }}
                        </UiButton>
                    </UiAlertAction>
                </UiAlert>

                <slot />
            </UiSidebarInset>
        </UiSidebarProvider>

        <ConfirmDialog />
    </main>
</template>

<script setup lang="ts">
const updateStatus = useUpdateStatus();

const breadcrumbLabels = {
    home: 'nav.home',
    settings: 'nav.settings',
    general: 'nav.general',
    hotkeys: 'nav.hotkeys',
    capture: 'nav.capture',
    recording: 'nav.recording',
    upload: 'nav.upload',
    drive: 'nav.drive',
    account: 'nav.account',
    updates: 'nav.updates',
};

const handleInstall = () => desktop('updates:install');
</script>
