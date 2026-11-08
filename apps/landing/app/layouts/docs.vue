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
                    <div class="overflow-y-auto px-2 pb-6">
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
            <UiButton
                variant="outline"
                class="w-full justify-start text-muted-foreground"
                @click="isSearchOpen = true"
            >
                <ISearch />
                <span class="flex-1 text-left">{{ $t('docs.search.button') }}</span>
                <UiKbd>{{ shortcut }}</UiKbd>
            </UiButton>
            <DocsSidebar />
        </aside>
        <div class="min-w-0 flex-1">
            <slot />
        </div>
    </div>

    <Footer />
    <DocsSearch />
</template>

<script setup lang="ts">
const isMenuOpen = ref(false);
const isSearchOpen = useState('docs-search-open', () => false);
const shortcut = 'Ctrl K';
</script>
