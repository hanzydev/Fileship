<template>
    <UiCard size="sm">
        <UiCardContent class="flex min-h-8 items-center gap-3">
            <UiSkeleton v-if="media" :class="cn('shrink-0 rounded-full', media)" />
            <div class="flex min-w-0 flex-1 flex-col gap-0.5">
                <TextBlock variant="small" class="w-full">
                    <UiSkeleton class="h-lh w-1/2 rounded-full" />
                </TextBlock>
                <TextBlock variant="muted" class="w-full text-xs">
                    <UiSkeleton class="h-lh w-1/3 rounded-full" />
                </TextBlock>
            </div>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
defineProps<{
    media?: string;
}>();
</script>
