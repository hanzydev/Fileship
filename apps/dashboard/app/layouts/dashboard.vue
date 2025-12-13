<template>
    <main>
        <div v-if="currentUser">
            <Transition
                enter-active-class="motion-safe:(animate-in fade-in zoom-in-95 slide-in-top-2)"
                leave-active-class="motion-safe:(animate-out fade-out zoom-out-95 slide-out-top-2)"
            >
                <div v-if="!currentUser.backupRestoreState">
                    <LayoutsActingAsUser />
                    <div flex="~ gap-6">
                        <LayoutsSidebar />

                        <slot />

                        <LayoutsUploadingFiles />
                    </div>
                </div>
            </Transition>
            <LayoutsBackupRestoring
                :open="!!currentUser.backupRestoreState"
                :state="currentUser.backupRestoreState!"
            />
        </div>
    </main>
</template>

<script setup lang="ts">
const embed = useEmbed();
const domains = useDomains();
const currentUser = useAuthUser();

onMounted(async () => {
    embed.value = await $fetch<IEmbed>('/api/users/@me/embed');
    domains.value = await $fetch<string[]>('/api/users/@me/domains');
});
</script>
