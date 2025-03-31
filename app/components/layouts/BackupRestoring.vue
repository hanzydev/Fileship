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
        <h2 max-w-2xl>Your data is now being restored from backup.</h2>
        <p text-neutral300 font-medium>
            You cannot perform any actions on this user until this process is completed.
        </p>
        <UiButton ring-none="!" cursor-default bg-fs-overlay-1="!">
            {{ state }}
        </UiButton>
        <UiProgressBar max-w-2xl />
    </UiModal>
</template>

<script setup lang="ts">
import { toast } from 'vue-sonner';

import type { BackupRestoreState } from '@prisma/client';

const { open, state: _state } = defineProps<{
    open: boolean;
    state: BackupRestoreState;
}>();

const state = computed(
    () =>
        ({
            Extracting: 'Extracting...  (this may take a while)',
            DeletingPreviousData: 'Deleting previous data...',
            RestoringData: 'Restoring data...',
        })[_state] || _state,
);

watch(
    () => open,
    (newValue, oldValue) => {
        if (!newValue && oldValue) {
            toast.success('Backup restored successfully');
        }
    },
);
</script>
