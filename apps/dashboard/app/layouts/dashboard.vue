<template>
    <div>
        <div v-if="currentUser" bg-fs-overlay-2>
            <Transition
                enter-active-class="motion-safe:(animate-in fade-in zoom-in-95 slide-in-top-2)"
                leave-active-class="motion-safe:(animate-out fade-out zoom-out-95 slide-out-top-2)"
            >
                <div v-if="!currentUser.backupRestoreState">
                    <LayoutsActingAsUser />
                    <LayoutsNavbar />
                    <LayoutsSidebar :items="sidebarItems" />

                    <LayoutsDashboard>
                        <slot />
                    </LayoutsDashboard>

                    <LayoutsUploadingFiles />
                </div>
            </Transition>
            <LayoutsBackupRestoring
                :open="!!currentUser.backupRestoreState"
                :state="currentUser.backupRestoreState!"
            />
        </div>
    </div>
</template>

<script setup lang="ts">
const embed = useEmbed();
const domains = useDomains();
const currentUser = useAuthUser();

const sidebarItems = computed(() => {
    const filtered = [
        {
            name: 'Home',
            icon: 'heroicons-solid:home',
            href: '/dashboard',
        },
    ];

    if (canUploadFiles(currentUser)) {
        filtered.push(
            {
                name: 'Files',
                icon: 'heroicons-solid:document-duplicate',
                href: '/dashboard/files',
            },
            {
                name: 'Folders',
                icon: 'heroicons-solid:folder',
                href: '/dashboard/folders',
            },
        );
    }

    if (canTakeNotes(currentUser)) {
        filtered.push({
            name: 'Notes',
            icon: 'heroicons:bookmark-solid',
            href: '/dashboard/notes',
        });
    }

    return filtered;
});

onMounted(async () => {
    embed.value = await $fetch<IEmbed>('/api/users/@me/embed');
    domains.value = await $fetch<string[]>('/api/users/@me/domains');
});
</script>
