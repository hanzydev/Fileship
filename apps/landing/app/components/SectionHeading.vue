<template>
    <div class="flex max-w-3xl flex-col gap-4">
        <TextBlock variant="small" class="font-mono text-chart-1">
            <slot name="eyebrow" />
        </TextBlock>
        <TextBlock variant="h2" class="border-none pb-0 text-4xl font-semibold lg:text-5xl">
            <slot name="title" />
        </TextBlock>
        <TextBlock v-if="slots.description" variant="muted" class="text-lg leading-relaxed">
            <slot name="description" />
        </TextBlock>
    </div>
</template>

<script setup lang="ts">
const slots = useSlots();
</script>
