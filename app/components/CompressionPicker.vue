<template>
    <UiDropdown v-model="isOpen" placement="top" pb0.5="!">
        <slot />
        <template #content>
            <div
                relative
                top-6
                w56
                overflow-y-auto
                rounded-lg
                bg-fs-overlay-2
                p1.5
                space-y-1
                ring="1 fs-accent"
            >
                <UiButton
                    v-for="(option, index) in compressions"
                    :key="index"
                    :icon="
                        option.value === compression.value
                            ? 'heroicons-solid:check'
                            : 'heroicons-solid:archive'
                    "
                    :variant="option.value === compression.value ? 'accent' : 'onOverlay'"
                    icon-size="20"
                    wfull
                    gap2
                    @click="
                        compression =
                            compression.value === option.value ? compressions[0]! : option;
                        isOpen = false;
                    "
                >
                    {{ option.label }}
                </UiButton>
            </div>
        </template>
    </UiDropdown>
</template>

<script setup lang="ts">
const isOpen = ref(false);

const compression = defineModel<{
    label: string;
    value: number;
}>({
    required: true,
});

const compressions = [
    {
        label: 'None',
        value: 0,
    },
    {
        label: 'Low (25%)',
        value: 25,
    },
    {
        label: 'Medium (50%)',
        value: 50,
    },
    {
        label: 'High (75%)',
        value: 75,
    },
    {
        label: 'Maximum (100%)',
        value: 100,
    },
];
</script>
