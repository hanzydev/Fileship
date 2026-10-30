<template>
    <UiDialog v-model:open="open">
        <UiDialogContent class="max-h-[90svh] overflow-y-auto sm:max-w-5xl">
            <UiDialogHeader>
                <UiDialogTitle>{{ $t('sharing.discord.dialogTitle') }}</UiDialogTitle>
                <UiDialogDescription>
                    {{ $t('sharing.discord.dialogDescription') }}
                </UiDialogDescription>
            </UiDialogHeader>

            <UiTabs v-model="kind">
                <UiTabsList class="w-full">
                    <UiTabsTrigger v-for="item in DISCORD_EMBED_KINDS" :key="item" :value="item">
                        {{ $t(`sharing.discord.kinds.${item}`) }}
                    </UiTabsTrigger>
                </UiTabsList>
            </UiTabs>

            <div class="grid gap-6 lg:grid-cols-[1fr_20rem]">
                <DiscordLayoutEditor
                    :key="kind"
                    v-model="draft[kind]"
                    :kind="kind"
                    :color="embed.color"
                    @reset="draft[kind] = defaultLayout(kind)"
                />
                <div class="flex flex-col gap-2 lg:sticky lg:top-0 lg:self-start">
                    <TextBlock variant="small">{{ $t('sharing.preview') }}</TextBlock>
                    <DiscordEmbedPreview
                        :embed="preview(kind, embed, draft[kind])"
                        :color="embed.color"
                    />
                    <TextBlock variant="muted">{{ $t('sharing.discord.limits') }}</TextBlock>
                </div>
            </div>

            <UiDialogFooter>
                <UiButton variant="outline" @click="open = false">
                    {{ $t('common.cancel') }}
                </UiButton>
                <UiButton @click="handleDone">{{ $t('sharing.discord.done') }}</UiButton>
            </UiDialogFooter>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
const { embed } = defineProps<{ embed: IEmbed }>();

const open = defineModel<boolean>('open', { required: true });
const discord = defineModel<IEmbed['discord']>('discord', { required: true });

const { defaultLayout, preview } = useDiscordPreview();

const kind = ref<DiscordEmbedKind>('file');

const copyLayouts = () =>
    Object.fromEntries(
        DISCORD_EMBED_KINDS.map((item) => [
            item,
            structuredClone(toRaw(discord.value[item])) ?? defaultLayout(item),
        ]),
    ) as Record<DiscordEmbedKind, DiscordLayout>;

const draft = ref(copyLayouts());

const handleDone = () => {
    for (const item of DISCORD_EMBED_KINDS) {
        const layout = toRaw(draft.value[item]);
        discord.value[item] =
            JSON.stringify(layout) === JSON.stringify(defaultLayout(item))
                ? null
                : structuredClone(layout);
    }

    open.value = false;
};

watch(open, (value) => {
    if (value) draft.value = copyLayouts();
});
</script>
