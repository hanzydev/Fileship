<template>
    <UiCard v-for="section in sections" :key="section.key">
        <UiCardHeader>
            <UiCardTitle>{{ section.title }}</UiCardTitle>
            <UiCardDescription>{{ section.description }}</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiFieldGroup>
                <template v-for="(field, index) in section.fields" :key="field.key">
                    <UiFieldSeparator v-if="index" />
                    <UiField
                        orientation="horizontal"
                        class="has-[>[data-slot=field-content]]:items-center"
                    >
                        <UiFieldContent>
                            <UiFieldLabel :for="`${section.key}-${field.key}`">
                                {{ field.label }}
                            </UiFieldLabel>
                            <UiFieldDescription>{{ field.description }}</UiFieldDescription>
                        </UiFieldContent>
                        <UiSwitch
                            :id="`${section.key}-${field.key}`"
                            :model-value="valueOf(section.key, field.key)"
                            :disabled="!settings"
                            @update:model-value="
                                updateSection(section.key, { [field.key]: $event })
                            "
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

const sections: {
    key: SectionKey;
    title: string;
    description: string;
    fields: { key: string; label: string; description: string }[];
}[] = [
    {
        key: 'capture',
        title: 'Region capture',
        description: 'How the screen looks while you pick an area.',
        fields: [
            {
                key: 'detectWindows',
                label: 'Detect windows',
                description: 'Highlight the window under the mouse so one click captures it.',
            },
            {
                key: 'detectControls',
                label: 'Detect controls',
                description: 'Also highlight buttons, panels and other parts inside windows.',
            },
            {
                key: 'showMagnifier',
                label: 'Magnifier',
                description: 'Show a zoomed view next to the mouse for pixel exact selections.',
            },
            {
                key: 'showCursor',
                label: 'Include the mouse cursor',
                description: 'Draw the cursor into screenshots.',
            },
            {
                key: 'playSound',
                label: 'Shutter sound',
                description: 'Play a camera shutter sound when a screenshot is taken.',
            },
        ],
    },
    {
        key: 'afterCapture',
        title: 'After capture',
        description: 'What happens to a screenshot once it is taken.',
        fields: [
            {
                key: 'upload',
                label: 'Upload to Fileship',
                description: 'Upload the screenshot with your upload settings.',
            },
            {
                key: 'annotate',
                label: 'Open the editor',
                description:
                    'Draw, blur and add text before the next steps. Region captures are edited while you select.',
            },
            {
                key: 'copyImage',
                label: 'Copy image',
                description: 'Put the image itself on the clipboard.',
            },
            {
                key: 'saveToFile',
                label: 'Save to this computer',
                description: 'Keep a copy in the folder from the upload settings.',
            },
        ],
    },
    {
        key: 'afterUpload',
        title: 'After upload',
        description: 'What happens once a file is on Fileship.',
        fields: [
            {
                key: 'copyUrl',
                label: 'Copy link',
                description: 'Put the link on the clipboard.',
            },
            {
                key: 'shortenUrl',
                label: 'Shorten link',
                description: 'Create a short link and use it instead.',
            },
            {
                key: 'openUrl',
                label: 'Open link',
                description: 'Open the link in your browser.',
            },
            {
                key: 'notify',
                label: 'Show a notification',
                description: 'Click the notification to open the link.',
            },
            {
                key: 'playSound',
                label: 'Upload sound',
                description: 'Play a short chime when an upload finishes.',
            },
        ],
    },
];

const valueOf = (section: SectionKey, key: string) =>
    (settings.value?.[section] as Record<string, boolean> | undefined)?.[key] ?? false;
</script>
