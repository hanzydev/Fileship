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

const source = inject<Ref<string> | undefined>('docsSource', undefined);
const root = inject<Ref<string> | undefined>('docsRoot', undefined);

const headingId = (hash: string) =>
    hash && `#${hash.slice(1).replace(/-+/g, '-').replace(/^-|-$/g, '').replace(/^(\d)/, '_$1')}`;

const url = computed(() => {
    const href = props.href ?? '';
    if (!source?.value || /^(?:[a-z]+:|\/)/i.test(href)) return href;
    if (href.startsWith('#')) return headingId(href);

    const folder = `/${source.value.slice(0, source.value.lastIndexOf('/') + 1)}`;
    const { pathname, hash } = new URL(href, `https://fileship.local${folder}`);

    if (pathname.startsWith('/docs/'))
        return `${root!.value}${pathname
            .slice('/docs'.length)
            .replace(/\.md$/, '')
            .replace(/\/index$/, '')}${headingId(hash)}`;

    const file = pathname.split('/').pop()!;
    const path = file.includes('.') ? pathname : `${pathname}.md`;

    return `https://github.com/hanzydev/Fileship/blob/main${path}${hash}`;
});

const isExternal = computed(() => /^https?:\/\//.test(url.value));
</script>
