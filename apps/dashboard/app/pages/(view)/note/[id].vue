<template>
    <div class="flex min-h-svh flex-col">
        <Head>
            <Title>{{ data?.title }}</Title>

            <template v-if="data?.embed?.enabled">
                <Meta name="theme-color" :content="data.embed.color" />
                <Meta property="og:title" :content="data.title" />
            </template>
        </Head>

        <ViewHeader :title="data!.title" :subtitle="dayjs(data!.createdAt).format('MMM D, YYYY')">
            <UiToggleGroup
                :model-value="previewMode ? 'preview' : 'raw'"
                type="single"
                variant="outline"
                size="sm"
                class="mr-1"
                @update:model-value="$event && (previewMode = $event === 'preview')"
            >
                <UiToggleGroupItem value="preview" aria-label="Preview">
                    <IEye />
                </UiToggleGroupItem>
                <UiToggleGroupItem value="raw" aria-label="Raw">
                    <ICode />
                </UiToggleGroupItem>
            </UiToggleGroup>
            <UiButton size="sm" @click="handleCopy">
                <ICopy />
                <span class="hidden sm:inline">Copy</span>
            </UiButton>
        </ViewHeader>

        <main class="mx-auto w-full max-w-3xl flex-1 px-4 py-8">
            <MarkdownRenderer v-if="previewMode" :content="data!.content" />
            <pre v-else class="font-mono text-sm break-all whitespace-pre-wrap">{{
                data!.content
            }}</pre>
        </main>
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

const route = useRoute();
const { $toast } = useNuxtApp();
const { copy } = useClipboard({ legacy: true });

const { data, error } = await useFetch<NoteData & { embed: Pick<IEmbed, 'enabled' | 'color'> }>(
    `/api/notes/${(route.params as any).id}`,
);

if (error.value?.statusCode === 404) {
    throw createError({ statusCode: 404, message: 'Note not found' });
} else if (error.value?.statusCode === 403) {
    throw createError({
        statusCode: 403,
        message: 'You do not have permission to access this page',
    });
}

const previewMode = ref(true);

const handleCopy = () => {
    copy(data.value!.content);
    $toast.success('Note copied to clipboard');
};
</script>
