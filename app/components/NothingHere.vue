<template>
    <div
        flex="~ col gap4 items-center justify-center"
        h48
        rounded-md
        bg-fs-overlay-2
        text-center
    >
        <div flex="~ gap4 items-center" text-slate300>
            <Icon :name="icon" size="32" />
            <h3>Nothing here</h3>
        </div>
        <p text-slate400>{{ message }}</p>
    </div>
</template>

<script setup lang="ts">
defineProps<{
    icon: string;
    message: string;
}>();
</script>
