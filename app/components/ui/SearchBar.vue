<template>
    <div flex="~ justify-between" relative>
        <Icon
            name="heroicons:magnifying-glass-16-solid"
            absolute
            left-3
            h6
            w6
            text-slate300
            translate-y-="1/2"
            top="1/2"
        />
        <input
            v-model="searchQuery"
            :class="inputClass"
            h12
            wfull
            rounded-md
            bg-fs3
            pl11
            pr4
            outline-none
            focus:ring-1
            focus:ring-fs-accent
            motion-safe:transition-shadow
            placeholder-slate-300
            type="text"
            :placeholder
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
