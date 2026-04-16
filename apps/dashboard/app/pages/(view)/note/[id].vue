<template>
    <div flex="~ col" h-screen>
        <Head>
            <Title>{{ data?.title }}</Title>

            <template v-if="data?.embed?.enabled">
                <Meta name="theme-color" :content="data.embed.color" />
                <Meta property="og:title" :content="data.title" />
            </template>
        </Head>

        <MarkdownRenderer :content="data!.content" />
    </div>
</template>

<script setup lang="ts">
const route = useRoute();

const { data, error } = await useFetch<NoteData & { embed: Pick<IEmbed, 'enabled' | 'color'> }>(
    `/api/notes/${route.params.id}`,
);

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
</script>
