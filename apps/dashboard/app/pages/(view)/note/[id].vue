<template>
    <div flex="~ col" h-screen>
        <Head>
            <Title>{{ data?.title }}</Title>

            <template v-if="data?.embed?.enabled">
                <Meta name="theme-color" :content="data.embed.color" />
                <Meta property="og:title" :content="data.title" />
            </template>
        </Head>

        <MarkdownRenderer v-if="previewMode" :content="data!.content" />
        <div v-else p8>
            <pre max-w-full break-all text-wrap w="[calc(100%-140px)]">{{ data!.content }}</pre>
        </div>

        <div
            flex="~ items-center justify-center gap-1"
            ring="1 fs-overlay-4"
            absolute
            right-8
            top-8
            rounded-2xl
            bg-fs-overlay-2
            p1
        >
            <UiButton
                alignment="center"
                variant="onOverlay"
                class="size-9 shrink-0 text-fs-muted-2 !rounded-xl !p0 hover:text-white"
                :icon="previewMode ? 'solar:text-bold' : 'solar:eye-bold'"
                icon-size="20"
                aria-label="previewMode ? 'Preview' : 'Raw'"
                @click="previewMode = !previewMode"
            />
            <UiButton
                alignment="center"
                variant="onOverlay"
                class="size-9 shrink-0 text-fs-muted-2 !rounded-xl !p0 hover:text-white"
                :icon="copied ? 'solar:clipboard-check-bold' : 'solar:clipboard-bold'"
                :icon-class="copied && 'text-green500!'"
                icon-size="20"
                @click="handleCopy"
            />
        </div>
    </div>
</template>

<script setup lang="ts">
const route = useRoute();

const { data, error } = await useFetch<NoteData & { embed: Pick<IEmbed, 'enabled' | 'color'> }>(
    `/api/notes/${route.params.id}`,
);

const previewMode = ref(true);
const { $toast } = useNuxtApp();
const { copied, copy } = useClipboard({ legacy: true });

if (error.value?.statusCode === 404) {
    throw createError({
        statusCode: 404,
        message: 'Note not found',
    });
} else if (error.value?.statusCode === 403) {
    throw createError({
        statusCode: 403,
        message: 'You do not have permission to access this page',
    });
}

const handleCopy = () => {
    copy(data.value!.content);
    $toast.success('Note copied to clipboard');
};
</script>
