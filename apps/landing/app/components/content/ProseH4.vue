<template>
    <h4 :id="id" class="group mt-6 mb-2 scroll-mt-24 text-lg font-semibold">
        <DocsAnchor :id="id"><slot /></DocsAnchor>
    </h4>
</template>

<script setup lang="ts">
defineProps<{ id?: string }>();
</script>
