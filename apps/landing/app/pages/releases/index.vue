<template>
    <div class="container mx-auto flex max-w-4xl flex-col gap-12 py-12 lg:py-16">
        <div class="flex flex-col gap-4">
            <TextBlock variant="h1" class="text-4xl font-semibold lg:text-5xl">
                {{ $t('releases.title') }}
            </TextBlock>
            <TextBlock variant="muted" class="text-lg leading-relaxed">
                {{ $t('releases.description') }}
            </TextBlock>
        </div>

        <section v-for="group in groups" :key="group.major" class="flex flex-col gap-4">
            <TextBlock variant="h2" class="border-none pb-0 text-2xl font-semibold">
                {{ $t('releases.major', { major: group.major }) }}
            </TextBlock>
            <div class="flex flex-col gap-3">
                <NuxtLink
                    v-for="release in group.releases"
                    :key="release.path"
                    :to="release.path"
                    class="flex flex-col gap-2 rounded-3xl border p-5 transition-colors hover:bg-muted/50"
                >
                    <div class="flex flex-wrap items-center gap-2">
                        <span class="font-mono text-lg font-semibold">{{ release.title }}</span>
                        <UiBadge v-if="release.path === latest">
                            {{ $t('releases.latest') }}
                        </UiBadge>
                        <UiBadge v-if="release.desktop" variant="secondary">
                            <IMonitor />
                            {{ $t('releases.desktop', { version: release.desktop }) }}
                        </UiBadge>
                        <span v-if="release.date" class="ml-auto text-sm text-muted-foreground">
                            {{ formatDate(release.date) }}
                        </span>
                    </div>
                    <TextBlock
                        v-if="release.description"
                        variant="muted"
                        class="line-clamp-2 leading-relaxed"
                    >
                        {{ release.description }}
                    </TextBlock>
                </NuxtLink>
            </div>
        </section>
    </div>
</template>

<script setup lang="ts">
const { t } = useI18n();
const formatDate = useReleaseDate();

const { data: releases } = await useAsyncData('releases', () =>
    queryCollection('releases')
        .order('order', 'DESC')
        .select('path', 'title', 'description', 'major', 'date', 'desktop')
        .all(),
);

const latest = computed(() => releases.value?.[0]?.path);

const groups = computed(() => {
    const byMajor = new Map<number, NonNullable<typeof releases.value>>();

    for (const release of releases.value ?? []) {
        byMajor.set(release.major, [...(byMajor.get(release.major) ?? []), release]);
    }

    return [...byMajor].map(([major, items]) => ({ major, releases: items }));
});

useSeoMeta({
    title: () => t('releases.title'),
    description: () => t('releases.description'),
});

defineOgImage('Fileship', {
    headline: t('seo.releasesHeadline'),
    title: t('releases.title'),
    description: t('releases.description'),
    tagline: t('seo.ogTagline'),
});
</script>
