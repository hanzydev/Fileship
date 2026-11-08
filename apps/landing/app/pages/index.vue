<template>
    <Head>
        <Title>Fileship</Title>
    </Head>

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
                <NuxtLink to="/docs" class="flex items-center gap-1.5">
                    {{ $t('hero.selfHost') }}
                    <IArrowRight class="size-5" />
                </NuxtLink>
            </UiButton>
            <UiButton size="lg" variant="secondary">
                <NuxtLink to="/docs" class="flex items-center gap-1.5">
                    <WindowsIcon class="size-5" />
                    {{ $t('hero.download') }}
                </NuxtLink>
            </UiButton>
        </template>
    </Hero>

    <Features />
    <Toolbox />
    <Compare />
    <GetStarted />
</template>
