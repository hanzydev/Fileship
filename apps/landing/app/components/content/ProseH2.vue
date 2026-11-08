<template>
    <h2
        :id="id"
        class="group mt-12 mb-4 scroll-mt-24 border-b pb-2 text-2xl font-semibold tracking-tight"
    >
        <DocsAnchor :id="id"><slot /></DocsAnchor>
    </h2>
</template>

<script setup lang="ts">
defineProps<{ id?: string }>();
</script>
