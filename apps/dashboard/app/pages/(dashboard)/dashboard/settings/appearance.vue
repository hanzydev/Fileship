<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('settings.appearance.theme') }}</UiCardTitle>
            <UiCardDescription>{{ $t('settings.appearance.themeDescription') }}</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiTabs v-model="colorMode.preference">
                <UiTabsList class="w-full">
                    <UiTabsTrigger value="light">
                        <ISun />
                        {{ $t('settings.appearance.light') }}
                    </UiTabsTrigger>
                    <UiTabsTrigger value="dark">
                        <IMoon />
                        {{ $t('settings.appearance.dark') }}
                    </UiTabsTrigger>
                    <UiTabsTrigger value="system">
                        <IMonitor />
                        {{ $t('settings.appearance.system') }}
                    </UiTabsTrigger>
                </UiTabsList>
            </UiTabs>
        </UiCardContent>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('settings.appearance.language') }}</UiCardTitle>
            <UiCardDescription>
                {{ $t('settings.appearance.languageDescription') }}
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiSelect
                :model-value="currentUser!.language ?? 'system'"
                @update:model-value="handleLanguageChange"
            >
                <UiSelectTrigger class="w-full sm:w-64">
                    <UiSelectValue />
                </UiSelectTrigger>
                <UiSelectContent>
                    <UiSelectItem value="system">{{ $t('labels.language.system') }}</UiSelectItem>
                    <UiSelectItem v-for="language in LANGUAGES" :key="language" :value="language">
                        {{ $t(`labels.language.${language}`) }}
                    </UiSelectItem>
                </UiSelectContent>
            </UiSelect>
        </UiCardContent>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('settings.appearance.accent') }}</UiCardTitle>
            <UiCardDescription>{{ $t('settings.appearance.accentDescription') }}</UiCardDescription>
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
                        <UiFieldTitle>{{ $t(`settings.appearance.accents.${accent}`) }}</UiFieldTitle>
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
const currentUser = useAuthUser();

const selectedAccent = computed(() =>
    currentTheme.value in accents ? currentTheme.value : 'Fileship',
);

const handleLanguageChange = (value: any) =>
    updateLanguage(value === 'system' ? null : (value as Language));
</script>
