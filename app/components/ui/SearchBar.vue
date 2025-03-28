<template>
    <div flex="~ justify-between" relative>
        <Icon
            name="heroicons:magnifying-glass-16-solid"
            absolute
            left-3
            h6
            w6
            text-neutral300
            translate-y-="1/2"
            top="1/2"
        />
        <input
            v-model="searchQuery"
            :class="inputClass"
            h12
            wfull
            rounded-xl
            bg-fs-overlay-2
            pl11
            pr4
            outline-none
            focus:border-fs-accent
            focus:border-solid
            motion-safe:transition-all
            placeholder-neutral300
            type="text"
            :placeholder
            border="1 dashed fs-overlay-4"
        />
    </div>
</template>

<script setup lang="ts">
defineProps<{
    placeholder: string;
    inputClass?: unknown;
}>();

const searchQuery = defineModel<string>({ required: true });
</script>
