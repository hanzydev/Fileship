<script setup lang="ts">
import type { HTMLAttributes } from 'vue';

const props = defineProps<{
    class?: HTMLAttributes['class'];
}>();
</script>

<template>
    <div
        data-slot="empty-title"
        :class="cn('text-lg font-medium tracking-tight cn-font-heading', props.class)"
    >
        <slot />
    </div>
</template>
