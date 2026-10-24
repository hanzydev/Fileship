<template>
    <Head>
        <Title>{{ $t('settings.nav.title') }}</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader
            :title="$t('settings.nav.title')"
            :description="$t('settings.nav.description')"
        />

        <div class="flex flex-col gap-4 lg:flex-row">
            <UiCard size="sm" class="hidden w-56 shrink-0 self-start lg:flex">
                <UiCardContent>
                    <nav class="flex flex-col gap-1">
                        <UiButton
                            v-for="section in sections"
                            :key="section.path"
                            :variant="route.path === section.path ? 'secondary' : 'ghost'"
                            class="justify-start"
                            as-child
                        >
                            <NuxtLink :to="section.path">
                                <component :is="section.icon" />
                                {{ section.label }}
                            </NuxtLink>
                        </UiButton>
                    </nav>
                </UiCardContent>
            </UiCard>

            <UiSelect :model-value="route.path" @update:model-value="navigateTo($event as string)">
                <UiSelectTrigger class="w-full lg:hidden">
                    <UiSelectValue />
                </UiSelectTrigger>
                <UiSelectContent>
                    <UiSelectItem
                        v-for="section in sections"
                        :key="section.path"
                        :value="section.path"
                    >
                        <component :is="section.icon" />
                        {{ section.label }}
                    </UiSelectItem>
                </UiSelectContent>
            </UiSelect>

            <div class="flex min-w-0 max-w-3xl flex-1 flex-col gap-4">
                <NuxtPage />
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
import {
    IArchive,
    IBrain,
    IHardDrive,
    IInbox,
    IMonitorSmartphone,
    IPalette,
    IPlug,
    IShare2,
    IShieldCheck,
    IUser,
} from '#components';

const { t } = useI18n();
const route = useRoute();
const currentUser = useAuthUser();
const publicSettings = usePublicSettings();

const sections = computed(() => [
    { label: t('settings.nav.profile'), path: '/dashboard/settings/profile', icon: IUser },
    { label: t('settings.nav.security'), path: '/dashboard/settings/security', icon: IShieldCheck },
    {
        label: t('settings.nav.sessions'),
        path: '/dashboard/settings/sessions',
        icon: IMonitorSmartphone,
    },
    ...(canUploadFiles(currentUser.value)
        ? [
              {
                  label: t('settings.nav.storage'),
                  path: '/dashboard/settings/storage',
                  icon: IHardDrive,
              },
          ]
        : []),
    { label: t('settings.nav.backups'), path: '/dashboard/settings/backups', icon: IArchive },
    { label: t('settings.nav.sharing'), path: '/dashboard/settings/sharing', icon: IShare2 },
    ...(canUploadFiles(currentUser.value)
        ? [{ label: t('settings.nav.inbox'), path: '/dashboard/settings/inbox', icon: IInbox }]
        : []),
    ...(canUploadFiles(currentUser.value) || canShortenLinks(currentUser.value)
        ? [
              {
                  label: t('settings.nav.integrations'),
                  path: '/dashboard/settings/integrations',
                  icon: IPlug,
              },
          ]
        : []),
    ...(publicSettings.value.aiEnabled && canUploadFiles(currentUser.value)
        ? [{ label: t('settings.nav.ai'), path: '/dashboard/settings/ai', icon: IBrain }]
        : []),
    { label: t('settings.nav.appearance'), path: '/dashboard/settings/appearance', icon: IPalette },
]);

definePageMeta({
    layout: 'dashboard',
    middleware: 'user-only',
});
</script>
