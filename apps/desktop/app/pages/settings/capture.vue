<template>
    <UiCard v-if="info?.session === 'wayland'">
        <UiCardHeader>
            <UiCardTitle>{{ $t('captureSettings.screens.title') }}</UiCardTitle>
            <UiCardDescription>{{ $t('captureSettings.screens.description') }}</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiButton variant="outline" @click="desktop('capture:chooseScreens')">
                <IMonitor />
                {{ $t('captureSettings.screens.choose') }}
            </UiButton>
        </UiCardContent>
    </UiCard>

    <UiCard v-for="section in sections" :key="section.key">
        <UiCardHeader>
            <UiCardTitle>{{ $t(`captureSettings.${section.key}.title`) }}</UiCardTitle>
            <UiCardDescription>
                {{ $t(`captureSettings.${section.key}.description`) }}
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiFieldGroup>
                <template v-for="(field, index) in section.fields" :key="field">
                    <UiFieldSeparator v-if="index" />
                    <UiField
                        orientation="horizontal"
                        class="has-[>[data-slot=field-content]]:items-center"
                    >
                        <UiFieldContent>
                            <UiFieldLabel :for="`${section.key}-${field}`">
                                {{ $t(`captureSettings.${section.key}.${field}.label`) }}
                            </UiFieldLabel>
                            <UiFieldDescription>
                                {{ $t(`captureSettings.${section.key}.${field}.description`) }}
                            </UiFieldDescription>
                        </UiFieldContent>
                        <UiSwitch
                            :id="`${section.key}-${field}`"
                            :model-value="valueOf(section.key, field)"
                            :disabled="!settings"
                            @update:model-value="updateSection(section.key, { [field]: $event })"
                        />
                    </UiField>
                </template>
            </UiFieldGroup>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
import type { AppInfo } from '../../../shared/ipc';

type SectionKey = 'capture' | 'afterCapture' | 'afterUpload';

const settings = useSettings();
const info = ref<AppInfo>();

const captureFields = computed(() => {
    const fields = ['detectWindows', 'detectControls', 'showMagnifier', 'showCursor', 'playSound'];
    if (info.value?.platform !== 'linux') return fields;

    return fields.filter(
        (field) =>
            field !== 'detectControls' &&
            (field !== 'detectWindows' || info.value!.session === 'x11'),
    );
});

const sections = computed<{ key: SectionKey; fields: string[] }[]>(() => [
    { key: 'capture', fields: captureFields.value },
    { key: 'afterCapture', fields: ['upload', 'copyImage', 'saveToFile'] },
    { key: 'afterUpload', fields: ['copyUrl', 'shortenUrl', 'openUrl', 'notify', 'playSound'] },
]);

const valueOf = (section: SectionKey, key: string) =>
    (settings.value?.[section] as Record<string, boolean> | undefined)?.[key] ?? false;

onMounted(async () => {
    info.value = await desktop('app:info');
});
</script>
