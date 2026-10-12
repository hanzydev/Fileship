<template>
    <div class="mb-2 flex flex-wrap items-end justify-between gap-4">
        <div class="flex flex-col gap-1">
            <TextBlock variant="h3" custom-tag="h1">{{ title }}</TextBlock>
            <TextBlock v-if="description" variant="muted">{{ description }}</TextBlock>
        </div>
        <div v-if="$slots.default" class="flex flex-wrap items-center gap-2">
            <slot />
        </div>
    </div>
</template>

<script setup lang="ts">
defineProps<{
    title: string;
    description?: string;
}>();
</script>
