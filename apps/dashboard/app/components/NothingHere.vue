<template>
    <div
        flex="~ col gap4 items-center justify-center"
        h48
        rounded-xl
        bg-fs-overlay-2
        text-center
        ring="1 fs-overlay-4"
    >
        <div flex="~ gap4 items-center" text-fs-muted-1>
            <Icon :name="icon" size="32" />
            <h3>Nothing here</h3>
        </div>
        <p text-fs-muted-2>{{ message }}</p>
    </div>
</template>

<script setup lang="ts">
defineProps<{
    icon: string;
    message: string;
}>();
</script>
