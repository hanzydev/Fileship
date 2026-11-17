<template>
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
                                {{
                                    $t(
                                        isUnavailable(section.key, field)
                                            ? `captureSettings.${section.key}.${field}.unavailable`
                                            : `captureSettings.${section.key}.${field}.description`,
                                    )
                                }}
                            </UiFieldDescription>
                        </UiFieldContent>
                        <UiSwitch
                            :id="`${section.key}-${field}`"
                            :model-value="valueOf(section.key, field)"
                            :disabled="!settings || isUnavailable(section.key, field)"
                            @update:model-value="updateSection(section.key, { [field]: $event })"
                        />
                    </UiField>
                </template>
            </UiFieldGroup>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
type SectionKey = 'capture' | 'afterCapture' | 'afterUpload';

const settings = useSettings();
const account = useAccount();

const sections: { key: SectionKey; fields: string[] }[] = [
    {
        key: 'capture',
        fields: [
            'redact',
            'detectWindows',
            'detectControls',
            'showMagnifier',
            'showCursor',
            'playSound',
        ],
    },
    { key: 'afterCapture', fields: ['upload', 'copyImage', 'saveToFile'] },
    { key: 'afterUpload', fields: ['copyUrl', 'shortenUrl', 'openUrl', 'notify', 'playSound'] },
];

const valueOf = (section: SectionKey, key: string) =>
    !isUnavailable(section, key) &&
    ((settings.value?.[section] as Record<string, boolean> | undefined)?.[key] ?? false);

const isUnavailable = (section: SectionKey, key: string) =>
    section === 'capture' && key === 'redact' && account.value?.aiEnabled === false;
</script>
