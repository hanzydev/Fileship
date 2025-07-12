<template>
    <UiButton
        alignment="center"
        border="2 dashed fs-accent"
        ring-none="!"
        wfull
        rounded-xl="!"
        :disabled
        @click="!disabled && $emit('action')"
    >
        <UiSpinner v-if="disabled" :size="56" stroke-fs-muted-3 />
        <Icon v-else :name="icon" size="56" text-fs-muted-3 />
    </UiButton>
</template>

<script setup lang="ts">
const { icon = 'heroicons-solid:plus' } = defineProps<{
    icon?: string;
    disabled?: boolean;
}>();

defineEmits<{
    action: [];
}>();
</script>
