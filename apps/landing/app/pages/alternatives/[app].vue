<template>
    <div class="bgGrid overflow-x-clip">
        <div class="container mx-auto flex max-w-5xl flex-col gap-16 py-16 lg:gap-20 lg:py-24">
            <div class="flex flex-col gap-6">
                <TextBlock variant="small" class="font-mono text-chart-1">
                    {{ $t('alternatives.eyebrow') }}
                </TextBlock>
                <TextBlock
                    variant="h1"
                    class="text-4xl! font-semibold! text-balance sm:text-5xl! lg:text-6xl!"
                >
                    {{ $t('alternatives.title', { app: name }) }}
                </TextBlock>
                <TextBlock variant="muted" class="max-w-3xl text-lg! leading-relaxed">
                    {{ $t(`alternatives.apps.${slug}.intro`) }}
                </TextBlock>
                <div class="flex flex-wrap items-center gap-3">
                    <UiButton size="lg" as-child>
                        <NuxtLink to="/docs/installation" class="flex items-center gap-1.5">
                            {{ $t('hero.selfHost') }}
                            <IArrowRight class="size-5" />
                        </NuxtLink>
                    </UiButton>
                    <UiButton size="lg" variant="secondary" as-child>
                        <NuxtLink to="/docs/migrating">
                            {{ $t('alternatives.migrateGuide') }}
                        </NuxtLink>
                    </UiButton>
                </div>
            </div>

            <section class="flex flex-col gap-6">
                <SectionHeading>
                    <template #title>{{ $t('alternatives.tableTitle') }}</template>
                    <template #description>
                        {{
                            $t('alternatives.tableDescription', {
                                version: $t(`alternatives.apps.${slug}.version`),
                            })
                        }}
                    </template>
                </SectionHeading>
                <CompareTable :apps="['Fileship', name]" />
            </section>

            <section class="flex flex-col gap-8">
                <SectionHeading>
                    <template #title>{{ $t('alternatives.addsTitle') }}</template>
                </SectionHeading>
                <div class="grid gap-4 sm:grid-cols-2">
                    <UiCard v-for="item in adds" :key="item">
                        <UiCardHeader>
                            <UiCardTitle>{{ $t(`alternatives.adds.${item}.title`) }}</UiCardTitle>
                            <UiCardDescription class="leading-relaxed">
                                {{ $t(`alternatives.adds.${item}.text`) }}
                            </UiCardDescription>
                        </UiCardHeader>
                    </UiCard>
                </div>
            </section>

            <section class="grid gap-10 md:grid-cols-2">
                <div class="flex flex-col gap-4">
                    <TextBlock variant="h3">
                        {{ $t('alternatives.aheadTitle', { app: name }) }}
                    </TextBlock>
                    <TextBlock variant="muted" class="leading-relaxed">
                        {{ $t(`alternatives.apps.${slug}.ahead`) }}
                    </TextBlock>
                </div>
                <div class="flex flex-col gap-4">
                    <TextBlock variant="h3">
                        {{ $t('alternatives.migrateTitle', { app: name }) }}
                    </TextBlock>
                    <TextBlock variant="muted" class="leading-relaxed">
                        {{ $t(`alternatives.apps.${slug}.migrate`) }}
                    </TextBlock>
                    <NuxtLink
                        to="/docs/migrating"
                        class="flex items-center gap-1.5 text-sm font-medium underline-offset-4 hover:underline"
                    >
                        {{ $t('alternatives.migrateGuide') }}
                        <IArrowRight class="size-4" />
                    </NuxtLink>
                </div>
            </section>

            <section class="flex flex-col gap-6">
                <SectionHeading>
                    <template #title>{{ $t('alternatives.faqTitle') }}</template>
                </SectionHeading>
                <dl class="flex flex-col divide-y rounded-4xl border">
                    <div v-for="item in faq" :key="item.key" class="flex flex-col gap-2 px-6 py-5">
                        <dt class="font-medium">{{ item.question }}</dt>
                        <dd class="text-muted-foreground leading-relaxed">{{ item.answer }}</dd>
                    </div>
                </dl>
            </section>
        </div>
    </div>
</template>

<script setup lang="ts">
const APPS: Record<string, string> = {
    zipline: 'Zipline',
    xbackbone: 'XBackBone',
    chibisafe: 'Chibisafe',
};

const route = useRoute();
const { t } = useI18n();

const slug = route.params.app as string;
const name = APPS[slug];

if (!name) {
    throw createError({ status: 404, statusText: 'Page not found', fatal: true });
}

const adds = ['desktop', 'harbor', 'ai', 'updates', 'sharing', 'backups'];

const faq = computed(() => [
    { key: 'free', question: t('alternatives.faq.free.q'), answer: t('alternatives.faq.free.a') },
    {
        key: 'sharex',
        question: t('alternatives.faq.sharex.q', { app: name }),
        answer: t('alternatives.faq.sharex.a'),
    },
    {
        key: 'migrate',
        question: t('alternatives.faq.migrate.q', { app: name }),
        answer: t(`alternatives.apps.${slug}.migrate`),
    },
    {
        key: 'requirements',
        question: t('alternatives.faq.requirements.q'),
        answer: t('alternatives.faq.requirements.a'),
    },
]);

useSeoMeta({
    title: () => t('alternatives.title', { app: name }),
    description: () => t(`alternatives.apps.${slug}.description`),
});

defineOgImage('Fileship', {
    headline: t('seo.comparisonHeadline'),
    title: t('alternatives.ogTitle', { app: name }),
    description: t(`alternatives.apps.${slug}.description`),
    tagline: t('seo.ogTagline'),
});

useSchemaOrg([
    defineWebPage({ '@type': 'FAQPage' }),
    ...faq.value.map((item) =>
        defineQuestion({ name: item.question, acceptedAnswer: item.answer }),
    ),
]);
</script>
