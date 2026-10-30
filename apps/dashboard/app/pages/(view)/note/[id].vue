<template>
    <div class="flex min-h-svh flex-col">
        <Head>
            <Title>{{ data?.title }}</Title>

            <template v-if="data?.embed?.enabled">
                <Meta name="theme-color" :content="data.embed.color" />
                <Meta property="og:title" :content="data.title" />
                <Meta v-if="data.excerpt" property="og:description" :content="data.excerpt" />
                <Meta v-if="data.publicUrl" property="og:url" :content="data.publicUrl" />
            </template>
        </Head>

        <ViewHeader
            :title="data!.title"
            :subtitle="date(data!.createdAt).format('ll')"
            content-class="max-w-3xl"
        >
            <UiToggleGroup
                type="single"
                variant="outline"
                size="sm"
                :model-value="previewMode ? 'preview' : 'raw'"
                @update:model-value="$event && (previewMode = $event === 'preview')"
            >
                <UiToggleGroupItem value="preview" :aria-label="$t('view.preview')">
                    <IEye />
                </UiToggleGroupItem>
                <UiToggleGroupItem value="raw" :aria-label="$t('view.raw')">
                    <ICode />
                </UiToggleGroupItem>
            </UiToggleGroup>
            <UiButton size="sm" @click="handleCopy">
                <ICopy />
                <span class="hidden sm:inline">{{ $t('view.copy') }}</span>
            </UiButton>
        </ViewHeader>

        <main class="mx-auto flex w-full max-w-3xl flex-1 flex-col p-4">
            <UiCard>
                <UiCardContent>
                    <MarkdownRenderer v-if="previewMode" :content="data!.content" />
                    <pre v-else class="font-mono text-sm break-all whitespace-pre-wrap">{{
                        data!.content
                    }}</pre>
                </UiCardContent>
            </UiCard>
        </main>
    </div>
</template>

<script setup lang="ts">
import type { NoteData } from '~/types/notes';

const route = useRoute();
const { $toast } = useNuxtApp();
const { t } = useI18n();
const date = useDate();
const { copy } = useClipboard({ legacy: true });

const { data, error } = await useFetch<
    NoteData & {
        embed: Pick<IEmbed, 'enabled' | 'color'>;
        excerpt: string;
        discordEmbed: string | null;
    }
>(`/api/notes/${(route.params as any).id}`);

useDiscordEmbed(() => data.value?.discordEmbed);

if (error.value?.status === 404) {
    throw createError({ status: 404, message: t('view.noteNotFound') });
} else if (error.value?.status === 403) {
    throw createError({
        status: 403,
        message: t('view.noPermission'),
    });
}

const previewMode = ref(true);

const handleCopy = () => {
    copy(data.value!.content);
    $toast.success(t('view.noteCopied'));
};
</script>
