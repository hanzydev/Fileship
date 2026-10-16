<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('account.server') }}</UiCardTitle>
            <UiCardDescription>{{ $t('account.serverDescription') }}</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <dl class="grid grid-cols-[auto_1fr] gap-x-6 gap-y-2 text-sm">
                <dt class="text-muted-foreground">{{ $t('account.name') }}</dt>
                <dd class="truncate text-right">{{ account?.siteName }}</dd>
                <dt class="text-muted-foreground">{{ $t('account.address') }}</dt>
                <dd class="truncate text-right">{{ account?.instanceUrl }}</dd>
                <dt class="text-muted-foreground">{{ $t('account.user') }}</dt>
                <dd class="truncate text-right">{{ account?.user.username }}</dd>
            </dl>
        </UiCardContent>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('account.session') }}</UiCardTitle>
            <UiCardDescription>{{ $t('account.sessionDescription') }}</UiCardDescription>
        </UiCardHeader>
        <UiCardFooter class="gap-2">
            <UiButton variant="outline" @click="handleOpenSessions">
                <IMonitorSmartphone />
                {{ $t('account.manageSessions') }}
            </UiButton>
            <UiButton variant="destructive" :disabled="isLoggingOut" @click="handleLogout">
                <UiSpinner v-if="isLoggingOut" />
                <ILogOut v-else />
                {{ $t('account.logOut') }}
            </UiButton>
        </UiCardFooter>
    </UiCard>
</template>

<script setup lang="ts">
const account = useAccount();
const { t } = useI18n();

const isLoggingOut = ref(false);

const handleOpenSessions = () => desktop('app:openDashboard', '/dashboard/settings/sessions');

const handleLogout = () =>
    askConfirmation({
        title: t('account.logOutTitle'),
        description: t('account.logOutDescription'),
        confirmText: t('account.logOut'),
        destructive: true,
        action: async () => {
            isLoggingOut.value = true;
            await desktop('account:logout');
            isLoggingOut.value = false;
        },
    });
</script>
