<template>
    <header
        class="sticky top-0 z-20 flex items-center gap-3 border-b bg-background/80 px-4 py-3 backdrop-blur"
    >
        <img src="/fileship.png" alt="" class="size-7 shrink-0" />
        <div class="flex min-w-0 flex-1 flex-col">
            <TextBlock variant="small" class="truncate">{{ title }}</TextBlock>
            <TextBlock v-if="subtitle" variant="muted" class="truncate text-xs">
                {{ subtitle }}
            </TextBlock>
        </div>
        <div class="flex shrink-0 items-center gap-1">
            <slot />
        </div>
    </header>
</template>

<script setup lang="ts">
defineProps<{
    title: string;
    subtitle?: string;
}>();
</script>
