<template>
    <UiCard size="sm">
        <UiCardContent>
            <UiSkeleton class="aspect-video" />
        </UiCardContent>
        <UiCardFooter class="gap-2">
            <div class="flex min-w-0 flex-1 flex-col gap-0.5">
                <TextBlock custom-tag="div" variant="small" class="w-full">
                    <UiSkeleton class="h-lh w-2/3 rounded-full" />
                </TextBlock>
                <TextBlock custom-tag="div" variant="muted" class="w-full text-xs">
                    <UiSkeleton class="h-lh w-1/3 rounded-full" />
                </TextBlock>
            </div>
            <UiSkeleton class="size-8 rounded-full" />
        </UiCardFooter>
    </UiCard>
</template>
