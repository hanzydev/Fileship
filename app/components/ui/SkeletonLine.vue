<template>
    <UiBaseSkeleton rounded-2xl :color-steps>
        <slot />
    </UiBaseSkeleton>
</template>

<script setup lang="ts">
const { colorSteps = ['var(--fs-overlay-3)', 'var(--fs-overlay-4)'] } = defineProps<{
    colorSteps?: [string, string];
}>();
</script>
