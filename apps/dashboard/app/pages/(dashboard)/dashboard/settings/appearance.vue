<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>Theme</UiCardTitle>
            <UiCardDescription>Light, dark or follow your system.</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiTabs v-model="colorMode.preference">
                <UiTabsList class="w-full">
                    <UiTabsTrigger value="light">
                        <ISun />
                        Light
                    </UiTabsTrigger>
                    <UiTabsTrigger value="dark">
                        <IMoon />
                        Dark
                    </UiTabsTrigger>
                    <UiTabsTrigger value="system">
                        <IMonitor />
                        System
                    </UiTabsTrigger>
                </UiTabsList>
            </UiTabs>
        </UiCardContent>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>Accent color</UiCardTitle>
            <UiCardDescription>Synced across all your devices.</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiRadioGroup
                :model-value="selectedAccent"
                class="grid grid-cols-2 gap-2 sm:grid-cols-4"
                @update:model-value="updateTheme($event as Accent)"
            >
                <UiFieldLabel
                    v-for="(color, accent) in accents"
                    :key="accent"
                    :for="`accent-${accent}`"
                >
                    <UiField orientation="horizontal">
                        <span class="size-4 shrink-0 rounded-full" :style="{ background: color }" />
                        <UiFieldTitle>{{ accent }}</UiFieldTitle>
                        <UiRadioGroupItem :id="`accent-${accent}`" :value="accent" />
                    </UiField>
                </UiFieldLabel>
            </UiRadioGroup>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
const colorMode = useColorMode();
const currentTheme = useTheme();

const selectedAccent = computed(() =>
    currentTheme.value in accents ? currentTheme.value : 'Fileship',
);
</script>
