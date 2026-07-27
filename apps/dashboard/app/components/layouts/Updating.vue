<template>
    <UiModal
        :model-value="open"
        max-hscreen
        max-wfull
        min-hscreen
        wscreen="!"
        rounded-none
        p8
        text-center
        space-y-4
        ring-0="!"
        bg-fs-background="!"
        flex="~ col items-center justify-center"
        :close-on-outer-click="false"
        :closable="false"
    >
        <h2 max-w-2xl>Fileship is updating...</h2>
        <p text-fs-muted-2 font-medium>
            Please wait while system updates to the latest version.
        </p>
        <UiButton ring-none="!" cursor-default bg-fs-overlay-1="!" rounded-2xl="!">
            {{ formattedState }}
        </UiButton>
        <UiProgressBar max-w-2xl />
    </UiModal>
</template>

<script setup lang="ts">
export type UpdateState = 'Pulling' | 'Recreating' | 'Success' | 'Error' | string;

const { open, state: _state } = defineProps<{
    open: boolean;
    state: UpdateState;
}>();

const formattedState = computed(
    () =>
        ({
            Pulling: 'Downloading new version...',
            Recreating: 'Recreating containers...',
            Success: 'Update completed successfully!',
            Error: 'Update failed!',
        })[_state] || _state,
);
</script>
