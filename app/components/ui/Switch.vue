<template>
    <label
        relative
        inline-flex="~ items-center"
        :class="disabled ? 'cursor-not-allowed' : 'cursor-pointer'"
    >
        <input
            v-model="checked"
            type="checkbox"
            value=""
            class="peer"
            sr-only
            :disabled
        />
        <div
            h6
            w11
            rounded-full
            bg-fs2
            after:absolute
            after:left-0.5
            after:top-0.5
            after:h-5
            after:w-5
            after:border
            after:rounded-full
            after:bg-white
            peer-checked:bg-fs-accent
            motion-safe:transition-colors
            after:content-empty
            peer-checked:after:translate-x-full
            peer-checked:after:border-white
            motion-safe:after:transition-all
            class="peer"
        ></div>
    </label>
</template>

<script setup lang="ts">
defineProps<{
    disabled?: boolean;
}>();

const checked = defineModel<boolean>({ required: true });
</script>
