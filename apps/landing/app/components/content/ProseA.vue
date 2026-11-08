<template>
    <NuxtLink
        :to="url"
        :target="isExternal ? '_blank' : target"
        :rel="isExternal ? 'noopener' : undefined"
        class="font-medium text-foreground underline decoration-muted-foreground/40 underline-offset-4 transition-colors hover:decoration-foreground"
    >
        <slot />
    </NuxtLink>
</template>

<script setup lang="ts">
const props = defineProps<{ href?: string; target?: string }>();

const route = useRoute();

const url = computed(() => {
    const href = props.href ?? '';
    if (!route.path.startsWith('/docs/') || !href.startsWith('../')) return href;

    const folder = route.path.slice(0, route.path.lastIndexOf('/') + 1);
    const { pathname, hash } = new URL(href, `https://fileship.local${folder}`);
    if (pathname.startsWith('/docs/')) return href;

    const file = pathname.split('/').pop()!;
    const path = file.includes('.') ? pathname : `${pathname}.md`;

    return `https://github.com/hanzydev/Fileship/blob/main${path}${hash}`;
});

const isExternal = computed(() => /^https?:\/\//.test(url.value));
</script>
