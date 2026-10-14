<template>
    <UiEmpty>
        <UiEmptyHeader>
            <UiEmptyMedia variant="icon">
                <component :is="icon" />
            </UiEmptyMedia>
            <UiEmptyTitle>{{ title }}</UiEmptyTitle>
            <UiEmptyDescription v-if="description">{{ description }}</UiEmptyDescription>
        </UiEmptyHeader>
        <UiEmptyContent v-if="$slots.default">
            <slot />
        </UiEmptyContent>
    </UiEmpty>
</template>

<script setup lang="ts">
defineProps<{
    icon: any;
    title: string;
    description?: string;
}>();
</script>
