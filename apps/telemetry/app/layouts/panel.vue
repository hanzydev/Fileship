<template>
    <main>
        <UiSidebarProvider storage-key="sidebar">
            <AppSidebar />
            <UiSidebarInset>
                <AppHeader :labels="breadcrumbLabels" />

                <slot />
            </UiSidebarInset>
        </UiSidebarProvider>
    </main>
</template>

<script setup lang="ts">
const breadcrumbLabels = {
    overview: 'Overview',
    versions: 'Versions',
    system: 'System',
    features: 'Features',
    instances: 'Instances',
};
</script>
