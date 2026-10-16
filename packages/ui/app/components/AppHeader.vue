<template>
    <header
        class="flex h-16 shrink-0 items-center gap-2 transition-[width,height] ease-linear group-has-data-[collapsible=icon]/sidebar-wrapper:h-12"
    >
        <div class="flex min-w-0 flex-1 items-center gap-2 px-4">
            <UiSidebarTrigger class="-ml-1" />

            <UiBreadcrumb class="min-w-0">
                <UiBreadcrumbList class="flex-nowrap">
                    <template v-for="(item, index) in breadcrumbItems" :key="item.to">
                        <UiBreadcrumbSeparator
                            v-if="index > 0"
                            :class="isMiddleCrumb(index) && 'hidden sm:block'"
                        />
                        <UiBreadcrumbItem
                            :class="cn('min-w-0', isMiddleCrumb(index) && 'hidden sm:inline-flex')"
                        >
                            <UiBreadcrumbLink v-if="index < breadcrumbItems.length - 1" as-child>
                                <NuxtLink :to="item.to" class="truncate">
                                    {{ item.label }}
                                </NuxtLink>
                            </UiBreadcrumbLink>
                            <UiBreadcrumbPage v-else class="truncate">
                                {{ item.label }}
                            </UiBreadcrumbPage>
                        </UiBreadcrumbItem>
                    </template>
                </UiBreadcrumbList>
            </UiBreadcrumb>
        </div>

        <div class="flex shrink-0 items-center gap-2 px-4">
            <slot />
            <UiButton
                variant="ghost"
                size="icon"
                :aria-label="$t('common.toggleTheme')"
                @click="handleToggleColorMode"
            >
                <ISun class="hidden dark:block" />
                <IMoon class="dark:hidden" />
            </UiButton>
        </div>
    </header>
</template>

<script setup lang="ts">
const { labels } = defineProps<{
    labels: Record<string, string>;
}>();

const colorMode = useColorMode();

const breadcrumbItems = useBreadcrumbs(labels);

const isMiddleCrumb = (index: number) => index > 0 && index < breadcrumbItems.value.length - 1;

const handleToggleColorMode = () => {
    colorMode.preference = colorMode.value === 'dark' ? 'light' : 'dark';
};
</script>
