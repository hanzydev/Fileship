<template>
    <Hero>
        <template #title>
            {{ $t('hero.titleLine1') }}
            <br />
            <span class="text-muted-foreground">{{ $t('hero.titleLine2') }}</span>
        </template>
        <template #description>
            {{ $t('hero.description') }}
        </template>
        <template #actions>
            <UiButton size="lg" as-child>
                <NuxtLink to="/docs/installation" class="flex items-center gap-1.5">
                    {{ $t('hero.selfHost') }}
                    <IArrowRight class="size-5" />
                </NuxtLink>
            </UiButton>
            <UiButton size="lg" variant="secondary" as-child>
                <a
                    :href="DESKTOP_RELEASES"
                    class="flex items-center gap-1.5"
                    @click.prevent="handleDownload"
                >
                    <WindowsIcon class="size-5" />
                    {{ $t('hero.download') }}
                </a>
            </UiButton>
        </template>
    </Hero>

    <Features />
    <Toolbox />
    <Compare />
    <GetStarted />
</template>

<script setup lang="ts">
const { t } = useI18n();

const DESKTOP_RELEASES = 'https://github.com/hanzydev/fileship-desktop/releases/latest';

const handleDownload = async () => {
    const release = await $fetch<{ assets: { name: string; browser_download_url: string }[] }>(
        'https://api.github.com/repos/hanzydev/fileship-desktop/releases/latest',
    ).catch(() => null);

    const installer = release?.assets.find((asset) => asset.name.endsWith('.exe'));

    window.location.href = installer?.browser_download_url ?? DESKTOP_RELEASES;
};

useSeoMeta({
    title: () => t('seo.homeTitle'),
    description: () => t('seo.homeDescription'),
});

defineOgImage('Fileship', {
    headline: t('seo.homeHeadline'),
    title: t('seo.homeOgTitle'),
    description: t('seo.homeOgDescription'),
    tagline: t('seo.ogTagline'),
});
</script>
