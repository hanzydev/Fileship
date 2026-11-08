<template>
    <UiButton
        variant="ghost"
        size="icon-xs"
        :aria-label="copied ? $t('common.copied') : $t('common.copy')"
        class="hover:bg-foreground/5 dark:hover:bg-foreground/10"
        @click="copy(value)"
    >
        <ICheck v-if="copied" />
        <ICopy v-else />
    </UiButton>
    <span aria-live="polite" class="sr-only">
        <template v-if="copied">{{ $t('common.copied') }}</template>
    </span>
</template>

<script setup lang="ts">
defineProps<{
    value: string;
}>();

const { copy, copied } = useClipboard({ copiedDuring: 2000, legacy: true });
</script>
