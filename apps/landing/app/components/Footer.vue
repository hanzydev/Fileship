<template>
    <footer class="border-t">
        <div class="container mx-auto flex flex-wrap justify-between gap-10 py-12 lg:py-14">
            <div class="flex max-w-xs flex-col gap-3">
                <div class="flex items-center gap-3">
                    <NuxtImg src="/fileship.png" width="24" height="24" alt="Fileship" />
                    <TextBlock variant="large">Fileship</TextBlock>
                </div>
                <TextBlock variant="muted" class="leading-relaxed">
                    {{ $t('footer.tagline') }}
                </TextBlock>
            </div>
            <div class="grid w-full grid-cols-2 gap-8 sm:flex sm:w-auto sm:gap-16">
                <nav v-for="group in groups" :key="group.title" class="flex flex-col gap-3">
                    <TextBlock variant="small">{{ $t(`footer.${group.title}`) }}</TextBlock>
                    <NuxtLink
                        v-for="link in group.links"
                        :key="link.title"
                        :to="link.href"
                        :target="link.href.startsWith('http') ? '_blank' : undefined"
                        class="text-sm text-muted-foreground transition-colors hover:text-foreground"
                    >
                        {{ $t(`footer.${link.title}`) }}
                    </NuxtLink>
                </nav>
            </div>
        </div>
        <div class="border-t">
            <div class="container mx-auto flex items-center justify-between gap-4 py-6">
                <TextBlock variant="muted">
                    {{ $t('footer.copyright', { years }) }}
                </TextBlock>
                <UiButton variant="ghost" size="icon-sm" as-child>
                    <NuxtLink
                        to="https://github.com/hanzydev/Fileship"
                        target="_blank"
                        :aria-label="$t('footer.github')"
                    >
                        <GitHubIcon />
                    </NuxtLink>
                </UiButton>
            </div>
        </div>
    </footer>
</template>

<script setup lang="ts">
const years = `2024–${new Date().getFullYear()}`;

const groups = [
    {
        title: 'product',
        links: [
            { title: 'features', href: '/#features' },
            {
                title: 'windowsApp',
                href: 'https://github.com/hanzydev/fileship-desktop/releases/latest',
            },
            {
                title: 'chrome',
                href: 'https://chromewebstore.google.com/detail/fileship/lbnlfnmlaipoflalhikladilalkoagnh',
            },
            { title: 'firefox', href: 'https://addons.mozilla.org/firefox/addon/fileship/' },
        ],
    },
    {
        title: 'docs',
        links: [
            { title: 'installation', href: '/docs/installation' },
            { title: 'configuration', href: '/docs/configuration' },
            { title: 'ai', href: '/docs/ai' },
            { title: 'migrating', href: '/docs/migrating' },
        ],
    },
    {
        title: 'compare',
        links: [
            { title: 'zipline', href: '/alternatives/zipline' },
            { title: 'xbackbone', href: '/alternatives/xbackbone' },
            { title: 'chibisafe', href: '/alternatives/chibisafe' },
        ],
    },
    {
        title: 'project',
        links: [
            { title: 'github', href: 'https://github.com/hanzydev/Fileship' },
            { title: 'releases', href: '/releases' },
            {
                title: 'security',
                href: 'https://github.com/hanzydev/Fileship/blob/main/SECURITY.md',
            },
            {
                title: 'contributing',
                href: 'https://github.com/hanzydev/Fileship/blob/main/CONTRIBUTING.md',
            },
        ],
    },
];
</script>
