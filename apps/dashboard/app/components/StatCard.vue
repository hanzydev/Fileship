<template>
    <UiCard>
        <UiCardHeader class="flex justify-between">
            <UiCardTitle class="text-muted-foreground">{{ title }}</UiCardTitle>
            <component :is="icon" class="size-6 text-primary" />
        </UiCardHeader>
        <UiCardContent class="flex flex-col gap-6 justify-between h-full">
            <div v-if="!isLoading" class="flex items-end gap-2">
                <TextBlock variant="h1" custom-tag="span" class="border-none">{{ data }}</TextBlock>
                <div
                    v-if="growth !== 0 && !isLoading"
                    :class="[
                        'flex items-center gap-0.5',
                        growth! > 0 ? 'text-green-500' : 'text-red-500',
                    ]"
                >
                    <span>{{ growth }}%</span>
                    <IArrowUpRight v-if="growth! > 0" />
                    <IArrowDownRight v-if="growth! < 0" />
                </div>
            </div>
            <UiSkeleton
                v-else
                class="h-14 rounded-full"
                :style="{
                    width: `${randomNumber(25, 75)}%`,
                }"
            />

            <UiCardDescription class="mt-auto">
                {{ legend }}
            </UiCardDescription>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
const { data, growth } = defineProps<{
    title: string;
    legend: string;
    icon: any;
    growth?: number;
    data?: number | string;
}>();

const isLoading = computed(() => data === undefined || growth === undefined);
</script>
