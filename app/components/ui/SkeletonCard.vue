<template>
    <UiBaseSkeleton wfull rounded-xl p4 border="1 fs-overlay-4" :color-steps>
        <slot />
    </UiBaseSkeleton>
</template>

<script setup lang="ts">
const { colorSteps = ['var(--fs-overlay-2)', 'var(--fs-overlay-3)'] } = defineProps<{
    colorSteps?: [string, string];
}>();
</script>
