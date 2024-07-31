<template>
    <UiDropdown v-model="isOpen" placement="top">
        <slot />
        <template #content>
            <div
                relative
                top-6
                h64
                w48
                overflow-y-auto
                rounded-lg
                bg-fs-overlay-2
                p1.5
                ring="1 fs-accent"
                space-y-1
            >
                <UiButton
                    v-for="(option, index) in expirations"
                    :key="index"
                    :icon="
                        option.value === expiration.value
                            ? 'heroicons-solid:check'
                            : 'heroicons-solid:clock'
                    "
                    :variant="
                        option.value === expiration.value ? 'accent' : 'primary'
                    "
                    icon-size="20"
                    wfull
                    gap2
                    @click="
                        expiration = option;
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

const expiration = defineModel<{
    label: string;
    value: number | null;
}>({
    required: true,
});

const expirations = [
    {
        label: 'Never',
        value: null,
    },
    {
        label: '5 Minutes',
        value: 300_000,
    },
    {
        label: '15 Minutes',
        value: 900_000,
    },
    {
        label: '30 Minutes',
        value: 1_800_000,
    },
    {
        label: '1 Hour',
        value: 3_600_000,
    },
    {
        label: '6 Hours',
        value: 21_600_000,
    },
    {
        label: '12 Hours',
        value: 43_200_000,
    },
    {
        label: '1 Day',
        value: 86_400_000,
    },
    {
        label: '1 Week',
        value: 604_800_000,
    },
    {
        label: '2 Week',
        value: 1_209_600_000,
    },
    {
        label: '1 Month',
        value: 2_592_000_000,
    },
    {
        label: '3 Months',
        value: 7_776_000_000,
    },
    {
        label: '6 Months',
        value: 15_552_000_000,
    },
    {
        label: '1 Year',
        value: 31_536_000_000,
    },
];

defineExpose({
    predictExpiration: (date: Date) => {
        const diff = date.getTime() - Date.now();

        return expirations.reduce((prev, curr) =>
            Math.abs(curr.value! - diff) < Math.abs(prev.value! - diff)
                ? curr
                : prev,
        );
    },
});
</script>
