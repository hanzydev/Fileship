<template>
    <div
        flex="~ items-center justify-center"
        border="2 dashed fs-accent"
        wfull
        rounded-md
        bg-fs3
        p4
        active:scale-95
        motion-safe:transition-transform
        :class="disabled ? 'cursor-not-allowed op75' : 'cursor-pointer'"
        @click="!disabled && $emit('action')"
    >
        <UiSpinner v-if="disabled" :size="56" stroke-slate400 />
        <Icon v-else :name="icon" size="56" text-slate400 />
    </div>
</template>

<script setup lang="ts">
withDefaults(
    defineProps<{
        icon?: string;
        disabled?: boolean;
    }>(),
    { icon: 'heroicons-solid:plus' },
);

defineEmits<{
    action: [];
}>();
</script>
