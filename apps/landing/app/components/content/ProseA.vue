<template>
    <NuxtLink
        :to="href"
        :target="isExternal ? '_blank' : target"
        :rel="isExternal ? 'noopener' : undefined"
        class="font-medium text-foreground underline decoration-muted-foreground/40 underline-offset-4 transition-colors hover:decoration-foreground"
    >
        <slot />
    </NuxtLink>
</template>

<script setup lang="ts">
const props = defineProps<{ href?: string; target?: string }>();

const isExternal = computed(() => /^https?:\/\//.test(props.href ?? ''));
</script>
