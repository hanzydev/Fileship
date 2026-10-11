<template>
    <UiCard size="sm">
        <UiCardContent>
            <UiSkeleton class="aspect-4/3" />
        </UiCardContent>
        <UiCardFooter class="flex-col items-start gap-0.5">
            <TextBlock variant="small" class="w-full">
                <UiSkeleton class="h-lh w-3/4 rounded-full" />
            </TextBlock>
            <TextBlock variant="muted" class="w-full text-xs">
                <UiSkeleton class="h-lh w-1/2 rounded-full" />
            </TextBlock>
        </UiCardFooter>
    </UiCard>
</template>
