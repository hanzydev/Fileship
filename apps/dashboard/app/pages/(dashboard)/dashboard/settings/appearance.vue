<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>Theme</UiCardTitle>
            <UiCardDescription>Light, dark or follow your system.</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiTabs v-model="colorMode.preference">
                <UiTabsList class="w-full">
                    <UiTabsTrigger value="light" class="flex-1">
                        <ISun />
                        Light
                    </UiTabsTrigger>
                    <UiTabsTrigger value="dark" class="flex-1">
                        <IMoon />
                        Dark
                    </UiTabsTrigger>
                    <UiTabsTrigger value="system" class="flex-1">
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
        <UiCardContent class="grid grid-cols-2 gap-2 sm:grid-cols-4">
            <UiButton
                v-for="(color, accent) in accents"
                :key="accent"
                :variant="selectedAccent === accent ? 'secondary' : 'outline'"
                class="justify-start"
                @click="updateTheme(accent)"
            >
                <span class="size-4 rounded-full" :style="{ background: color }" />
                {{ accent }}
                <ICheck v-if="selectedAccent === accent" class="ml-auto" />
            </UiButton>
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
