<template>
    <div class="border-b">
        <div class="flex items-center justify-between gap-4 py-4 container mx-auto">
            <NuxtLink to="/" class="flex items-center gap-3">
                <NuxtImg src="/fileship.png" width="28" height="28" alt="Fileship" />
                <TextBlock variant="h4">Fileship</TextBlock>
            </NuxtLink>
            <nav class="hidden items-center gap-1 lg:flex">
                <UiButton v-for="item in items" :key="item.href" variant="ghost" as-child>
                    <NuxtLink :to="item.href">{{ $t(item.title) }}</NuxtLink>
                </UiButton>
            </nav>
            <div class="flex items-center gap-3">
                <UiButton variant="outline" class="hidden sm:inline-flex" as-child>
                    <NuxtLink to="https://github.com/hanzydev/Fileship" target="_blank">
                        <GitHubIcon />
                        {{ $t('nav.github') }}
                    </NuxtLink>
                </UiButton>
                <UiButton class="hidden sm:inline-flex" as-child>
                    <NuxtLink to="#getStarted">{{ $t('nav.getStarted') }}</NuxtLink>
                </UiButton>

                <UiSheet v-model:open="isMenuOpen">
                    <UiSheetTrigger as-child>
                        <UiButton
                            variant="outline"
                            size="icon"
                            class="lg:hidden"
                            :aria-label="$t('nav.openMenu')"
                        >
                            <IMenu />
                        </UiButton>
                    </UiSheetTrigger>
                    <UiSheetContent
                        class="data-[side=right]:w-full data-[side=right]:sm:max-w-xs"
                        @close-auto-focus="
                            pendingHref && ($event.preventDefault(), handleMenuClosed())
                        "
                    >
                        <UiSheetHeader>
                            <UiSheetTitle>{{ $t('nav.menu') }}</UiSheetTitle>
                        </UiSheetHeader>
                        <nav class="flex flex-col gap-1 px-4">
                            <UiButton
                                v-for="item in items"
                                :key="item.href"
                                variant="ghost"
                                class="justify-start"
                                @click="handleNavigate(item.href)"
                            >
                                {{ $t(item.title) }}
                            </UiButton>
                        </nav>
                        <UiSheetFooter class="sm:hidden">
                            <UiButton variant="outline" as-child>
                                <NuxtLink to="https://github.com/hanzydev/Fileship" target="_blank">
                                    <GitHubIcon />
                                    {{ $t('nav.github') }}
                                </NuxtLink>
                            </UiButton>
                            <UiButton @click="handleNavigate('#getStarted')">
                                {{ $t('nav.getStarted') }}
                            </UiButton>
                        </UiSheetFooter>
                    </UiSheetContent>
                </UiSheet>
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
const isMenuOpen = ref(false);

const items = [
    {
        title: 'nav.features',
        href: '#features',
    },
    {
        title: 'nav.windowsApp',
        href: '#windowsApp',
    },
    {
        title: 'nav.compare',
        href: '#compare',
    },
    {
        title: 'nav.getStarted',
        href: '#getStarted',
    },
    {
        title: 'nav.docs',
        href: '/docs',
    },
];

const pendingHref = ref('');

const handleNavigate = (href = '') => {
    pendingHref.value = href;
    isMenuOpen.value = false;
};

const handleMenuClosed = () => {
    navigateTo(pendingHref.value);
    pendingHref.value = '';
};
</script>
