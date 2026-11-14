<template>
    <div v-if="page" class="flex gap-10">
        <article class="min-w-0 flex-1 py-8 lg:py-10">
            <div class="mb-6 flex flex-wrap items-center justify-between gap-3">
                <UiBreadcrumb>
                    <UiBreadcrumbList>
                        <UiBreadcrumbItem>
                            <UiBreadcrumbLink as-child>
                                <NuxtLink :to="version.root">{{ $t('docs.title') }}</NuxtLink>
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
            <nav :aria-label="$t('docs.onThisPage')" class="sticky top-0 flex flex-col gap-3 py-10">
                <TextBlock variant="small">{{ $t('docs.onThisPage') }}</TextBlock>
                <div class="relative flex flex-col border-l">
                    <span
                        :class="
                            cn(
                                'absolute -left-px w-0.5 rounded-full bg-sidebar-primary transition-[top,height,opacity] duration-300 ease-out',
                                !indicator && 'opacity-0',
                            )
                        "
                        :style="indicator"
                    />
                    <a
                        v-for="link in toc"
                        :key="link.id"
                        ref="links"
                        :href="`#${link.id}`"
                        :class="
                            cn(
                                'py-1 text-sm text-muted-foreground transition-colors hover:text-foreground',
                                link.depth > 2 ? 'pl-7' : 'pl-4',
                                activeId === link.id && 'text-foreground',
                            )
                        "
                        @click="activeId = link.id"
                    >
                        {{ link.text }}
                    </a>
                </div>
            </nav>
        </aside>
    </div>
</template>

<script setup lang="ts">
const route = useRoute();
const { version } = useDocsVersion();

const { data: page } = await useAsyncData(`docs-${route.path}`, () =>
    queryCollection(version.value.collection).path(route.path).first(),
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

const source = computed(() => `docs${page.value!.stem.slice(version.value.root.length - 1)}`);

const editUrl = computed(
    () => `https://github.com/hanzydev/Fileship/edit/${version.value.branch}/${source.value}.md`,
);

provide('docsSource', source);
provide('docsVersion', version);

const { copy, copied } = useClipboard({ legacy: true });

const handleCopy = () => copy(page.value!.rawbody);

const activeId = ref('');
const links = useTemplateRef<HTMLAnchorElement[]>('links');

const indicator = computed(() => {
    const index = toc.value.findIndex((link) => link.id === activeId.value);
    const element = links.value?.[index];

    return element && { top: `${element.offsetTop}px`, height: `${element.offsetHeight}px` };
});

let frame = 0;

const updateActive = () => {
    cancelAnimationFrame(frame);
    frame = requestAnimationFrame(() => {
        const headings = toc.value
            .map((link) => document.getElementById(link.id))
            .filter((element) => !!element);

        const atBottom =
            window.innerHeight + window.scrollY >= document.documentElement.scrollHeight - 4;

        const current = atBottom
            ? headings.at(-1)
            : headings.findLast((element) => element.getBoundingClientRect().top <= 120);

        activeId.value = current?.id ?? headings[0]?.id ?? '';
    });
};

useEventListener('scroll', updateActive, { passive: true });

onMounted(updateActive);

watch(
    () => route.path,
    () => nextTick(updateActive),
);

const { t } = useI18n();

const description = computed(() => page.value!.description || t('docs.description'));

useSeoMeta({
    title: () => page.value!.title,
    description,
    ogTitle: () => page.value!.title,
    ogDescription: description,
});

defineOgImage('Fileship', {
    headline: group.value ? t(`docs.groups.${group.value}`) : t('seo.docsHeadline'),
    title: page.value.title,
    description: description.value,
    tagline: t('seo.ogTagline'),
});

definePageMeta({
    layout: 'docs',
});
</script>
