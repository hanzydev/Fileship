<template>
    <div class="grid gap-3 sm:grid-cols-2">
        <UiField>
            <UiFieldLabel :for="`${id}-url`">{{ $t('sharing.discord.mediaUrl') }}</UiFieldLabel>
            <UiInput :id="`${id}-url`" v-model="media.url" maxlength="2048" />
        </UiField>
        <UiField>
            <UiFieldLabel :for="`${id}-description`">
                {{ $t('sharing.discord.altText') }}
            </UiFieldLabel>
            <UiInput :id="`${id}-description`" v-model="media.description" maxlength="1024" />
        </UiField>
        <UiField orientation="horizontal" class="sm:col-span-2">
            <UiSwitch :id="`${id}-spoiler`" v-model="media.spoiler" />
            <UiFieldLabel :for="`${id}-spoiler`">{{ $t('sharing.discord.spoiler') }}</UiFieldLabel>
        </UiField>
    </div>
</template>

<script setup lang="ts">
const { id } = defineProps<{ id: string }>();

const media = defineModel<DiscordMedia>({ required: true });
</script>
