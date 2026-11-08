<template>
    <NuxtLoadingIndicator color="var(--primary)" :height="2" />
    <AppToaster />

    <Head>
        <Title>Fileship</Title>
    </Head>
    <NuxtLayout>
        <NuxtPage />
    </NuxtLayout>
</template>

<script setup lang="ts">
const { locale } = useI18n();

useHead({
    htmlAttrs: {
        lang: locale,
    },
    titleTemplate: (title) =>
        title
            ? title === 'Fileship' || title.match(/\.|:/)
                ? title
                : `${title} · Fileship`
            : 'Fileship',
});

useHead({
    script: [
        {
            innerHTML:
                "if (!matchMedia('(prefers-reduced-motion: reduce)').matches) document.documentElement.classList.add('motion')",
            tagPosition: 'head',
        },
    ],
    meta: [
        { key: 'theme-color-accent', name: 'theme-color', content: '#4f39f6', tagPriority: 'low' },
    ],
});

useSeoMeta({
    ogType: 'website',
    twitterCard: 'summary_large_image',
});

useSchemaOrg([
    defineWebSite({ name: 'Fileship' }),
    defineSoftwareApp({
        name: 'Fileship',
        operatingSystem: 'Linux, Docker, Windows',
        applicationCategory: 'BusinessApplication',
        license: 'https://www.gnu.org/licenses/agpl-3.0.html',
        offers: { price: 0, priceCurrency: 'USD' },
    }),
]);
</script>
