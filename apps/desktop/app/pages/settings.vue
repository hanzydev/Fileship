<template>
    <Head>
        <Title>{{ $t('settings.title') }}</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader :title="$t('settings.title')" :description="$t('settings.description')" />

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
                                {{ $t(section.label) }}
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
                        {{ $t(section.label) }}
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
    IAnchor,
    ICamera,
    IDownload,
    IKeyboard,
    ISlidersHorizontal,
    IUpload,
    IUser,
    IVideo,
} from '#components';

const route = useRoute();

const sections = [
    { label: 'settings.general', path: '/settings/general', icon: ISlidersHorizontal },
    { label: 'settings.hotkeys', path: '/settings/hotkeys', icon: IKeyboard },
    { label: 'settings.capture', path: '/settings/capture', icon: ICamera },
    { label: 'settings.recording', path: '/settings/recording', icon: IVideo },
    { label: 'settings.upload', path: '/settings/upload', icon: IUpload },
    { label: 'settings.harbor', path: '/settings/harbor', icon: IAnchor },
    { label: 'settings.account', path: '/settings/account', icon: IUser },
    { label: 'settings.updates', path: '/settings/updates', icon: IDownload },
];

definePageMeta({
    layout: 'desktop',
    redirect: '/settings/general',
});
</script>
