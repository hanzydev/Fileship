<template>
    <main>
        <UiSidebarProvider storage-key="sidebar">
            <AppSidebar />
            <UiSidebarInset>
                <AppHeader :labels="breadcrumbLabels" />

                <UiAlert v-if="updateStatus.state === 'ready'" class="mx-4 mb-4 w-auto">
                    <IDownload />
                    <UiAlertTitle>Fileship {{ updateStatus.version }} is ready</UiAlertTitle>
                    <UiAlertDescription>
                        Restart to update now, or it installs the next time you quit.
                    </UiAlertDescription>
                    <UiAlertAction>
                        <UiButton size="sm" @click="handleInstall">Restart now</UiButton>
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
    settings: 'Settings',
    general: 'General',
    hotkeys: 'Hotkeys',
    capture: 'Capture',
    upload: 'Upload',
    account: 'Account',
    updates: 'Updates',
};

const handleInstall = () => desktop('updates:install');
</script>
