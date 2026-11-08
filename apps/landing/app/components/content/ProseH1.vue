<template>
    <h1 :id="id" class="mb-4 scroll-mt-24 text-4xl font-semibold tracking-tight">
        <slot />
    </h1>
</template>

<script setup lang="ts">
defineProps<{ id?: string }>();
</script>
