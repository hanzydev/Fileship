<template>
    <img
        :src
        :alt
        select-none
        rounded-full
        bg-fs3
        :class="[size === 'xs' && 'h8 w8', size === 'sm' && 'h9 w9']"
        draggable="false"
    />
</template>

<script setup lang="ts">
defineProps<{
    size: 'xs' | 'sm';
    src: string;
    alt: string;
}>();
</script>
