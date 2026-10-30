<template>
    <div v-if="embed" class="flex overflow-hidden rounded-xl bg-muted">
        <div class="w-1 shrink-0" :style="{ backgroundColor: color }" />
        <div class="flex min-w-0 flex-1 flex-col gap-3 p-4 text-sm">
            <UiBadge v-if="embed.component.spoiler" variant="secondary" class="self-start">
                <IEyeOff />
                {{ $t('sharing.discord.spoiler') }}
            </UiBadge>

            <template v-for="(component, index) in embed.component.components" :key="index">
                <div v-if="component.type === 10" class="flex flex-col gap-1">
                    <template
                        v-for="(part, partIndex) in textParts(component.content)"
                        :key="partIndex"
                    >
                        <MarkdownRenderer
                            :class="part.isSubtext && 'text-xs text-muted-foreground'"
                            :content="part.text"
                        />
                    </template>
                </div>

                <div v-else-if="component.type === 9" class="flex items-start gap-3">
                    <div class="flex min-w-0 flex-1 flex-col gap-1">
                        <MarkdownRenderer
                            v-for="(text, textIndex) in component.components"
                            :key="textIndex"
                            :content="text.content"
                        />
                    </div>
                    <div
                        v-if="component.accessory.type === 11"
                        class="flex size-16 shrink-0 items-center justify-center rounded-lg bg-background"
                    >
                        <IEyeOff
                            v-if="component.accessory.spoiler"
                            class="size-5 text-muted-foreground"
                        />
                        <IImage v-else class="size-5 text-muted-foreground" />
                    </div>
                    <UiButton v-else as="span" variant="secondary" size="sm">
                        {{ buttonText(component.accessory) }}
                        <IExternalLink />
                    </UiButton>
                </div>

                <div
                    v-else-if="component.type === 12"
                    :class="cn('grid gap-1', component.items.length > 1 && 'grid-cols-2')"
                >
                    <div
                        v-for="(item, itemIndex) in component.items"
                        :key="itemIndex"
                        class="flex aspect-video items-center justify-center rounded-lg bg-background"
                    >
                        <IEyeOff v-if="item.spoiler" class="size-8 text-muted-foreground" />
                        <IImage v-else class="size-8 text-muted-foreground" />
                    </div>
                </div>

                <UiSeparator
                    v-else-if="component.type === 14"
                    :class="
                        cn(
                            component.spacing === 2 && 'my-2',
                            component.divider === false && 'opacity-0',
                        )
                    "
                />

                <div v-else-if="component.type === 1" class="flex flex-wrap gap-2">
                    <UiButton
                        v-for="(button, buttonIndex) in component.components"
                        :key="buttonIndex"
                        as="span"
                        variant="secondary"
                        size="sm"
                    >
                        {{ buttonText(button) }}
                        <IExternalLink />
                    </UiButton>
                </div>
            </template>
        </div>
    </div>
    <EmptyState
        v-else
        :icon="ILayoutTemplate"
        :title="$t('sharing.discord.emptyPreview')"
        :description="$t('sharing.discord.emptyPreviewDescription')"
    />
</template>

<script setup lang="ts">
import { ILayoutTemplate } from '#components';

const { embed, color } = defineProps<{
    embed: any;
    color: string;
}>();

const textParts = (content: string) =>
    content.split('\n').reduce<{ isSubtext: boolean; text: string }[]>((parts, line) => {
        const isSubtext = line.startsWith('-# ');
        const text = isSubtext ? line.slice(3) : line;
        const last = parts.at(-1);

        if (last && !isSubtext && !last.isSubtext) last.text += `\n${text}`;
        else parts.push({ isSubtext, text });

        return parts;
    }, []);

const buttonText = (button: { label?: string; emoji?: { name: string } }) =>
    [button.emoji?.name, button.label].filter(Boolean).join(' ');
</script>
