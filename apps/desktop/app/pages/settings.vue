<template>
    <Head>
        <Title>Settings</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader title="Settings" description="How Fileship behaves on this computer." />

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
import { IDownload, ISlidersHorizontal, IUser } from '#components';

const route = useRoute();

const sections = [
    { label: 'General', path: '/settings/general', icon: ISlidersHorizontal },
    { label: 'Account', path: '/settings/account', icon: IUser },
    { label: 'Updates', path: '/settings/updates', icon: IDownload },
];

definePageMeta({
    layout: 'desktop',
    redirect: '/settings/general',
});
</script>
