<template>
    <Head>
        <Title>Settings</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-6 p-4 pt-0">
        <PageHeader title="Settings" description="Manage your account and preferences." />

        <div class="flex flex-col gap-6 lg:flex-row">
            <nav class="-ml-[13px] hidden w-56 shrink-0 flex-col gap-1 lg:flex">
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

            <div class="flex min-w-0 max-w-3xl flex-1 flex-col gap-6">
                <NuxtPage />
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
import {
    IArchive,
    IBrain,
    IInbox,
    IMonitorSmartphone,
    IPalette,
    IPlug,
    IShare2,
    IShieldCheck,
    IUser,
} from '#components';

const route = useRoute();
const runtimeConfig = useRuntimeConfig();

const sections = computed(() => [
    { label: 'Profile', path: '/dashboard/settings/profile', icon: IUser },
    { label: 'Security', path: '/dashboard/settings/security', icon: IShieldCheck },
    { label: 'Sessions', path: '/dashboard/settings/sessions', icon: IMonitorSmartphone },
    { label: 'Backups', path: '/dashboard/settings/backups', icon: IArchive },
    { label: 'Sharing', path: '/dashboard/settings/sharing', icon: IShare2 },
    { label: 'Inbox', path: '/dashboard/settings/inbox', icon: IInbox },
    { label: 'Integrations', path: '/dashboard/settings/integrations', icon: IPlug },
    ...(runtimeConfig.public.aiEnabled
        ? [{ label: 'AI', path: '/dashboard/settings/ai', icon: IBrain }]
        : []),
    { label: 'Appearance', path: '/dashboard/settings/appearance', icon: IPalette },
]);

definePageMeta({
    layout: 'dashboard',
    middleware: 'user-only',
});
</script>
