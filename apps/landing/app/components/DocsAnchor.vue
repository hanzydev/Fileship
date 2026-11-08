<template>
    <a v-if="id" :href="`#${id}`" class="inline-flex items-center gap-2">
        <slot />
        <ILink
            aria-hidden="true"
            class="size-4 text-muted-foreground opacity-0 transition-opacity group-hover:opacity-100"
        />
    </a>
    <slot v-else />
</template>

<script setup lang="ts">
defineProps<{ id?: string }>();
</script>
