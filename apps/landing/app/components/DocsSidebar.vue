<template>
    <nav :aria-label="$t('docs.navigation')" class="flex flex-col gap-6">
        <div v-for="group in groups" :key="group.key" class="flex flex-col gap-0.5">
            <TextBlock variant="small" class="px-3 pb-2 text-xs text-muted-foreground">
                {{ $t(`docs.groups.${group.key}`) }}
            </TextBlock>
            <NuxtLink
                v-for="page in group.pages"
                :key="page.path"
                :to="page.path"
                :aria-current="page.path === route.path ? 'page' : undefined"
                :class="
                    cn(
                        'rounded-full px-3 py-1.5 text-sm text-muted-foreground transition-colors hover:bg-muted hover:text-foreground',
                        page.path === route.path && 'bg-muted font-medium text-foreground',
                    )
                "
                @click="emit('navigate')"
            >
                {{ page.title }}
            </NuxtLink>
        </div>
    </nav>
</template>

<script setup lang="ts">
const emit = defineEmits<{ navigate: [] }>();

const route = useRoute();
const { groups } = await useDocsNav();
</script>
