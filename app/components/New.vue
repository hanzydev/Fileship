<template>
    <UiButton
        alignment="center"
        border="2 dashed fs-accent"
        ring-none="!"
        wfull
        :class="disabled ? 'cursor-not-allowed op75' : 'cursor-pointer'"
        @click="!disabled && $emit('action')"
    >
        <UiSpinner v-if="disabled" :size="56" stroke-slate400 />
        <Icon v-else :name="icon" size="56" text-slate400 />
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
