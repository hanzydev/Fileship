<template>
    <div class="grid gap-3 sm:grid-cols-[1fr_5rem_1fr]">
        <UiField>
            <UiFieldLabel :for="`${id}-label`">
                {{ $t('sharing.discord.buttonLabel') }}
            </UiFieldLabel>
            <UiInput :id="`${id}-label`" v-model="button.label" maxlength="80" />
        </UiField>
        <UiField>
            <UiFieldLabel :for="`${id}-emoji`">{{ $t('sharing.discord.emoji') }}</UiFieldLabel>
            <UiInput :id="`${id}-emoji`" v-model="button.emoji" maxlength="32" />
        </UiField>
        <UiField>
            <UiFieldLabel :for="`${id}-url`">{{ $t('sharing.discord.buttonUrl') }}</UiFieldLabel>
            <UiInput :id="`${id}-url`" v-model="button.url" maxlength="2048" />
        </UiField>
    </div>
</template>

<script setup lang="ts">
const { id } = defineProps<{ id: string }>();

const button = defineModel<DiscordButton>({ required: true });
</script>
