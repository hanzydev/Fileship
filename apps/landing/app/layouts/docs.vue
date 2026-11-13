<template>
    <Navbar />

    <div class="sticky top-0 z-10 border-b bg-background/85 backdrop-blur-md lg:hidden">
        <div class="container mx-auto flex items-center gap-2 py-2">
            <UiSheet v-model:open="isMenuOpen">
                <UiSheetTrigger as-child>
                    <UiButton variant="ghost" size="sm">
                        <IPanelLeft />
                        {{ $t('docs.menu') }}
                    </UiButton>
                </UiSheetTrigger>
                <UiSheetContent
                    side="left"
                    class="data-[side=left]:w-full data-[side=left]:sm:max-w-xs"
                >
                    <UiSheetHeader>
                        <UiSheetTitle>{{ $t('docs.title') }}</UiSheetTitle>
                    </UiSheetHeader>
                    <div class="flex flex-col gap-6 overflow-y-auto px-2 pb-6">
                        <DocsVersionSelect :paths="paths!" @navigate="isMenuOpen = false" />
                        <DocsSidebar @navigate="isMenuOpen = false" />
                    </div>
                </UiSheetContent>
            </UiSheet>
            <div class="flex-1" />
            <UiButton
                variant="ghost"
                size="icon-sm"
                :aria-label="$t('docs.search.title')"
                @click="isSearchOpen = true"
            >
                <ISearch />
            </UiButton>
        </div>
    </div>

    <div class="container mx-auto flex gap-10">
        <aside
            class="sticky top-0 hidden h-dvh w-64 shrink-0 flex-col gap-6 overflow-y-auto border-r py-8 pr-4 lg:flex"
        >
            <div class="flex flex-col gap-3">
                <DocsVersionSelect :paths="paths!" />
                <UiButton
                    variant="outline"
                    class="w-full justify-start text-muted-foreground"
                    @click="isSearchOpen = true"
                >
                    <ISearch />
                    <span class="flex-1 text-left">{{ $t('docs.search.button') }}</span>
                    <UiKbd>{{ shortcut }}</UiKbd>
                </UiButton>
            </div>
            <DocsSidebar />
        </aside>
        <div class="min-w-0 flex-1">
            <UiAlert v-if="version.key === 'experimental'" class="mt-8 lg:mt-10">
                <IFlaskConical class="text-chart-1" />
                <UiAlertTitle>{{ $t('docs.experimental.title') }}</UiAlertTitle>
                <UiAlertDescription>
                    <p>
                        {{ $t('docs.experimental.description', { version: version.version }) }}
                        <NuxtLink :to="paths!.stable">
                            {{ $t('docs.experimental.stable') }}
                        </NuxtLink>
                    </p>
                </UiAlertDescription>
            </UiAlert>
            <slot />
        </div>
    </div>

    <Footer />
    <DocsSearch />
</template>

<script setup lang="ts">
const route = useRoute();
const { versions, version } = useDocsVersion();

const { data: paths } = await useAsyncData(
    () => `docs-versions-${route.path}`,
    async () => {
        const page = route.path.slice(version.value.root.length);

        const entries = await Promise.all(
            versions
                .filter((item) => item !== version.value)
                .map(async (item) => {
                    const path = `${item.root}${page}`;
                    const found = await queryCollection(item.collection)
                        .path(path)
                        .select('path')
                        .first();

                    return [item.key, found ? path : item.root] as const;
                }),
        );

        return Object.fromEntries(entries);
    },
);

const isMenuOpen = ref(false);
const isSearchOpen = useState('docs-search-open', () => false);
const shortcut = 'Ctrl K';
</script>
