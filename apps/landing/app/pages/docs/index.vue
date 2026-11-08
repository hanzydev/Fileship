<template>
    <div class="flex flex-col gap-12 py-8 lg:py-10">
        <div class="flex max-w-3xl flex-col gap-4">
            <TextBlock variant="h1" class="text-4xl font-semibold lg:text-5xl">
                {{ $t('docs.title') }}
            </TextBlock>
            <TextBlock variant="muted" class="text-lg leading-relaxed">
                {{ $t('docs.description') }}
            </TextBlock>
        </div>

        <div class="grid grid-cols-1 gap-4 md:grid-cols-2 2xl:grid-cols-6">
            <UiCard v-for="(group, index) in sections" :key="group.key" :class="spanOf(index)">
                <UiCardHeader class="gap-3">
                    <span
                        class="flex size-10 items-center justify-center rounded-full bg-muted text-chart-1"
                    >
                        <component :is="ICONS[group.key]" class="size-5" />
                    </span>
                    <UiCardTitle class="text-lg font-semibold">
                        {{ $t(`docs.groups.${group.key}`) }}
                    </UiCardTitle>
                    <UiCardDescription class="text-base leading-relaxed">
                        {{ $t(`docs.groupDescriptions.${group.key}`) }}
                    </UiCardDescription>
                </UiCardHeader>
                <UiCardContent class="mt-auto flex flex-col gap-1">
                    <NuxtLink
                        v-for="page in group.pages.slice(0, 5)"
                        :key="page.path"
                        :to="page.path"
                        class="flex items-center gap-2 rounded-full py-1 text-sm text-muted-foreground transition-colors hover:text-foreground"
                    >
                        <IArrowRight class="size-3.5 shrink-0" />
                        <span class="truncate">{{ page.title }}</span>
                    </NuxtLink>
                </UiCardContent>
            </UiCard>
        </div>
    </div>
</template>

<script setup lang="ts">
import type { Component } from 'vue';

import {
    IArrowRightLeft,
    IBlocks,
    ILayers,
    ILifeBuoy,
    IRocket,
    IServer,
    ISettings,
    ISmartphone,
    IWebhook,
} from '#components';

const ICONS: Record<string, Component> = {
    gettingStarted: IRocket,
    proxy: IServer,
    configuration: ISettings,
    features: ILayers,
    apps: ISmartphone,
    integrations: IWebhook,
    updating: IArrowRightLeft,
    troubleshooting: ILifeBuoy,
    more: IBlocks,
};

const { t } = useI18n();
const { groups } = await useDocsNav();

const sections = computed(() => groups.value.filter((group) => group.key !== 'more'));

const spanOf = (index: number) => {
    const count = sections.value.length;
    const lastRow = count % 3;

    return cn(
        count % 2 === 1 && index === count - 1 && 'md:col-span-2',
        index < count - lastRow || !lastRow
            ? '2xl:col-span-2'
            : lastRow === 2
              ? '2xl:col-span-3'
              : '2xl:col-span-6',
    );
};

useSeoMeta({
    title: () => t('docs.title'),
    description: () => t('docs.description'),
});

definePageMeta({
    layout: 'docs',
});
</script>
