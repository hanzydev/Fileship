<template>
    <div v-if="page" class="flex gap-10">
        <article class="min-w-0 flex-1 py-8 lg:py-10">
            <div class="mb-6 flex flex-wrap items-center justify-between gap-3">
                <UiBreadcrumb>
                    <UiBreadcrumbList>
                        <UiBreadcrumbItem>
                            <UiBreadcrumbLink as-child>
                                <NuxtLink to="/docs">{{ $t('docs.title') }}</NuxtLink>
                            </UiBreadcrumbLink>
                        </UiBreadcrumbItem>
                        <template v-if="group">
                            <UiBreadcrumbSeparator />
                            <UiBreadcrumbItem>
                                {{ $t(`docs.groups.${group}`) }}
                            </UiBreadcrumbItem>
                        </template>
                        <UiBreadcrumbSeparator />
                        <UiBreadcrumbItem>
                            <UiBreadcrumbPage>{{ page.title }}</UiBreadcrumbPage>
                        </UiBreadcrumbItem>
                    </UiBreadcrumbList>
                </UiBreadcrumb>
                <div class="flex items-center gap-2">
                    <UiButton variant="outline" size="sm" @click="handleCopy">
                        <ICheck v-if="copied" />
                        <ICopy v-else />
                        {{ copied ? $t('docs.copied') : $t('docs.copyPage') }}
                    </UiButton>
                    <UiButton variant="ghost" size="sm" as-child>
                        <NuxtLink :to="editUrl" target="_blank">
                            <IPencil />
                            {{ $t('docs.edit') }}
                        </NuxtLink>
                    </UiButton>
                </div>
            </div>

            <ContentRenderer :value="page" />

            <nav
                v-if="previous || next"
                :aria-label="$t('docs.pagination')"
                class="mt-14 grid gap-3 border-t pt-6 sm:grid-cols-2"
            >
                <NuxtLink
                    v-if="previous"
                    :to="previous.path"
                    class="flex flex-col gap-1 rounded-3xl border p-4 transition-colors hover:bg-muted/50"
                >
                    <span class="flex items-center gap-1 text-sm text-muted-foreground">
                        <IChevronLeft class="size-4" />
                        {{ $t('docs.previous') }}
                    </span>
                    <span class="font-medium">{{ previous.title }}</span>
                </NuxtLink>
                <NuxtLink
                    v-if="next"
                    :to="next.path"
                    class="flex flex-col items-end gap-1 rounded-3xl border p-4 text-right transition-colors hover:bg-muted/50 sm:col-start-2"
                >
                    <span class="flex items-center gap-1 text-sm text-muted-foreground">
                        {{ $t('docs.next') }}
                        <IChevronRight class="size-4" />
                    </span>
                    <span class="font-medium">{{ next.title }}</span>
                </NuxtLink>
            </nav>
        </article>

        <aside v-if="toc.length" class="hidden w-56 shrink-0 xl:block">
            <nav :aria-label="$t('docs.onThisPage')" class="sticky top-0 flex flex-col gap-1 py-10">
                <TextBlock variant="small" class="mb-2">{{ $t('docs.onThisPage') }}</TextBlock>
                <a
                    v-for="link in toc"
                    :key="link.id"
                    :href="`#${link.id}`"
                    :class="
                        cn(
                            'border-l-2 py-1 text-sm text-muted-foreground transition-colors hover:text-foreground',
                            link.depth > 2 ? 'pl-6' : 'pl-3',
                            activeId === link.id && 'border-sidebar-primary text-foreground',
                        )
                    "
                >
                    {{ link.text }}
                </a>
            </nav>
        </aside>
    </div>
</template>

<script setup lang="ts">
const route = useRoute();

const { data: page } = await useAsyncData(`docs-${route.path}`, () =>
    queryCollection('docs').path(route.path).first(),
);

if (!page.value) {
    throw createError({ status: 404, statusText: 'Page not found', fatal: true });
}

const { groups, flat } = await useDocsNav();

const group = computed(
    () => groups.value.find((g) => g.pages.some((p) => p.path === route.path))?.key,
);

const position = computed(() => flat.value.findIndex((p) => p.path === route.path));
const previous = computed(() => flat.value[position.value - 1]);
const next = computed(() => (position.value < 0 ? undefined : flat.value[position.value + 1]));

const toc = computed(() =>
    (page.value!.body.toc?.links ?? []).flatMap((link) => [link, ...(link.children ?? [])]),
);

const editUrl = computed(
    () => `https://github.com/hanzydev/Fileship/edit/main/${page.value!.stem}.md`,
);

provide(
    'docsSource',
    computed(() => page.value!.stem),
);

const { copy, copied } = useClipboard({ legacy: true });

const handleCopy = () => copy(page.value!.rawbody);

const activeId = ref('');

let observer: IntersectionObserver | undefined;

onMounted(() => {
    observer = new IntersectionObserver(
        (entries) => {
            const visible = entries.find((entry) => entry.isIntersecting);
            if (visible) activeId.value = visible.target.id;
        },
        { rootMargin: '0px 0px -70% 0px' },
    );

    toc.value.forEach((link) => {
        const element = document.getElementById(link.id);
        if (element) observer!.observe(element);
    });
});

onBeforeUnmount(() => observer?.disconnect());

useSeoMeta({
    title: () => page.value!.title,
    description: () => page.value!.description,
    ogTitle: () => page.value!.title,
    ogDescription: () => page.value!.description,
});

definePageMeta({
    layout: 'docs',
});
</script>
