<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>Server</UiCardTitle>
            <UiCardDescription>The Fileship server this computer uploads to.</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <dl class="grid grid-cols-[auto_1fr] gap-x-6 gap-y-2 text-sm">
                <dt class="text-muted-foreground">Name</dt>
                <dd class="truncate text-right">{{ account?.siteName }}</dd>
                <dt class="text-muted-foreground">Address</dt>
                <dd class="truncate text-right">{{ account?.instanceUrl }}</dd>
                <dt class="text-muted-foreground">User</dt>
                <dd class="truncate text-right">{{ account?.user.username }}</dd>
            </dl>
        </UiCardContent>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>Session</UiCardTitle>
            <UiCardDescription>
                This computer appears as "Fileship for Windows" in your sessions, where you can also
                revoke it.
            </UiCardDescription>
        </UiCardHeader>
        <UiCardFooter class="gap-2">
            <UiButton variant="outline" @click="handleOpenSessions">
                <IMonitorSmartphone />
                Manage sessions
            </UiButton>
            <UiButton variant="destructive" :disabled="isLoggingOut" @click="handleLogout">
                <UiSpinner v-if="isLoggingOut" />
                <ILogOut v-else />
                Log out
            </UiButton>
        </UiCardFooter>
    </UiCard>
</template>

<script setup lang="ts">
const account = useAccount();

const isLoggingOut = ref(false);

const handleOpenSessions = () => desktop('app:openDashboard', '/dashboard/settings/sessions');

const handleLogout = () =>
    askConfirmation({
        title: 'Log out of Fileship?',
        description: 'Hotkeys stop uploading until you log in again.',
        confirmText: 'Log out',
        destructive: true,
        action: async () => {
            isLoggingOut.value = true;
            await desktop('account:logout');
            isLoggingOut.value = false;
        },
    });
</script>
