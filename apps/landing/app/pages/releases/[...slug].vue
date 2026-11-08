<template>
    <div v-if="release" class="container mx-auto flex max-w-3xl flex-col gap-8 py-12 lg:py-16">
        <UiBreadcrumb>
            <UiBreadcrumbList>
                <UiBreadcrumbItem>
                    <UiBreadcrumbLink as-child>
                        <NuxtLink to="/releases">{{ $t('releases.title') }}</NuxtLink>
                    </UiBreadcrumbLink>
                </UiBreadcrumbItem>
                <UiBreadcrumbSeparator />
                <UiBreadcrumbItem>
                    <UiBreadcrumbPage>{{ release.title }}</UiBreadcrumbPage>
                </UiBreadcrumbItem>
            </UiBreadcrumbList>
        </UiBreadcrumb>

        <div class="flex flex-col gap-4 border-b pb-8">
            <div class="flex flex-wrap items-center gap-3">
                <TextBlock variant="h1" class="font-mono text-4xl font-semibold lg:text-5xl">
                    {{ release.title }}
                </TextBlock>
                <UiBadge v-if="isLatest">{{ $t('releases.latest') }}</UiBadge>
            </div>
            <div class="flex flex-wrap items-center gap-x-4 gap-y-2 text-sm text-muted-foreground">
                <span v-if="release.date" class="flex items-center gap-1.5">
                    <ICalendar class="size-4" />
                    {{ formatDate(release.date) }}
                </span>
                <span v-if="release.desktop" class="flex items-center gap-1.5">
                    <IMonitor class="size-4" />
                    {{ $t('releases.desktop', { version: release.desktop }) }}
                </span>
            </div>
            <div class="flex flex-wrap gap-2">
                <UiButton variant="outline" size="sm" as-child>
                    <NuxtLink :to="githubUrl" target="_blank">
                        <GitHubIcon />
                        {{ $t('releases.github') }}
                    </NuxtLink>
                </UiButton>
                <UiButton v-if="release.compare" variant="ghost" size="sm" as-child>
                    <NuxtLink :to="release.compare" target="_blank">
                        <IGitCompare />
                        {{ $t('releases.compare') }}
                    </NuxtLink>
                </UiButton>
            </div>
        </div>

        <ContentRenderer :value="release" />

        <nav
            v-if="older || newer"
            :aria-label="$t('releases.more')"
            class="grid gap-3 border-t pt-6 sm:grid-cols-2"
        >
            <NuxtLink
                v-if="older"
                :to="older.path"
                class="flex flex-col gap-1 rounded-3xl border p-4 transition-colors hover:bg-muted/50"
            >
                <span class="flex items-center gap-1 text-sm text-muted-foreground">
                    <IChevronLeft class="size-4" />
                    {{ $t('releases.older') }}
                </span>
                <span class="font-mono font-medium">{{ older.title }}</span>
            </NuxtLink>
            <NuxtLink
                v-if="newer"
                :to="newer.path"
                class="flex flex-col items-end gap-1 rounded-3xl border p-4 text-right transition-colors hover:bg-muted/50 sm:col-start-2"
            >
                <span class="flex items-center gap-1 text-sm text-muted-foreground">
                    {{ $t('releases.newer') }}
                    <IChevronRight class="size-4" />
                </span>
                <span class="font-mono font-medium">{{ newer.title }}</span>
            </NuxtLink>
        </nav>
    </div>
</template>

<script setup lang="ts">
const route = useRoute();
const formatDate = useReleaseDate();

const { data: release } = await useAsyncData(`release-${route.path}`, () =>
    queryCollection('releases').path(route.path).first(),
);

if (!release.value) {
    throw createError({ status: 404, statusText: 'Release not found', fatal: true });
}

const { data: around } = await useAsyncData(`release-around-${route.path}`, () =>
    Promise.all([
        queryCollection('releases')
            .where('order', '<', release.value!.order)
            .order('order', 'DESC')
            .select('path', 'title')
            .first(),
        queryCollection('releases')
            .where('order', '>', release.value!.order)
            .order('order', 'ASC')
            .select('path', 'title')
            .first(),
        queryCollection('releases').order('order', 'DESC').select('path').first(),
    ]),
);

const older = computed(() => around.value?.[0]);
const newer = computed(() => around.value?.[1]);
const isLatest = computed(() => around.value?.[2]?.path === route.path);

const githubUrl = computed(
    () => `https://github.com/hanzydev/Fileship/releases/tag/${release.value!.title}`,
);

useSeoMeta({
    title: () => `Fileship ${release.value!.title}`,
    description: () => release.value!.description,
    ogTitle: () => `Fileship ${release.value!.title}`,
    ogDescription: () => release.value!.description,
});

const { t } = useI18n();

defineOgImage('Fileship', {
    headline: release.value.date
        ? t('seo.releaseHeadlineDate', { date: formatDate(release.value.date) })
        : t('seo.releaseHeadline'),
    title: `Fileship ${release.value.title}`,
    description: release.value.description,
    tagline: t('seo.ogTagline'),
});
</script>
