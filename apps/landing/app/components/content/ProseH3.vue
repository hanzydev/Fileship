<template>
    <h3 :id="id" class="group mt-8 mb-3 scroll-mt-24 text-xl font-semibold tracking-tight">
        <DocsAnchor :id="id"><slot /></DocsAnchor>
    </h3>
</template>

<script setup lang="ts">
defineProps<{ id?: string }>();
</script>
