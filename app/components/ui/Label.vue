<template>
    <label
        block
        wfit
        text-left
        uppercase
        font-semibold="!"
        :class="error ? 'text-red-500' : 'text-slate300'"
    >
        <span text-sm="!">
            <slot />
        </span>
        {{ ' ' }}
        <span v-if="required && !error" text-red-500 text-sm="!">*</span>
        <span v-if="error" normal-case italic text-sm="!">- {{ error }}</span>
    </label>
</template>

<script setup lang="ts">
const props = defineProps<{ error?: string; required?: boolean }>();
const { error, required } = toRefs(props);
</script>
