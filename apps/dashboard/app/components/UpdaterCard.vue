<template>
    <UiCard size="sm" class="group-data-[collapsible=icon]:hidden">
        <UiCardHeader>
            <UiCardTitle class="flex items-center gap-2">
                <ISparkles class="size-4 text-primary" />
                Update available
            </UiCardTitle>
            <UiCardDescription>
                v{{ status.version }} → v{{ status.latestVersion }}
            </UiCardDescription>
        </UiCardHeader>
        <UiCardFooter class="gap-2">
            <UiButton v-if="status.updaterAvailable" size="sm" class="flex-1" @click="handleUpdate">
                Update
            </UiButton>
            <UiButton
                size="sm"
                :variant="status.updaterAvailable ? 'outline' : 'default'"
                :class="!status.updaterAvailable && 'flex-1'"
                as-child
            >
                <a :href="status.url" target="_blank" rel="noopener noreferrer">Changelog</a>
            </UiButton>
        </UiCardFooter>
    </UiCard>

    <UiDialog :open="isUpdating">
        <UiDialogContent :show-close-button="false" class="sm:max-w-sm" @interact-outside.prevent>
            <UiDialogHeader class="items-center text-center">
                <UiSpinner v-if="state !== 'Success' && state !== 'Error'" class="size-8" />
                <ICircleCheck v-else-if="state === 'Success'" class="size-8 text-primary" />
                <ICircleX v-else class="size-8 text-destructive" />
                <UiDialogTitle>Updating Fileship</UiDialogTitle>
                <UiDialogDescription>{{ states[state] || state }}</UiDialogDescription>
            </UiDialogHeader>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
const { status } = defineProps<{
    status: { version: string; url: string; latestVersion: string; updaterAvailable: boolean };
}>();

const { $toast } = useNuxtApp();

const isUpdating = ref(false);
const state = ref('Pulling');

const states: Record<string, string> = {
    Pulling: 'Downloading the new version...',
    Recreating: 'Recreating containers, the dashboard will be back shortly...',
    Success: 'Update completed, reloading...',
    Error: 'Update failed.',
};

const finish = (success: boolean) => {
    state.value = success ? 'Success' : 'Error';

    if (success) {
        $toast.success('Fileship updated successfully!');
        setTimeout(() => window.location.reload(), 2_000);
    } else {
        $toast.error('An unknown error occurred while updating Fileship');
        setTimeout(() => (isUpdating.value = false), 2_000);
    }
};

const pollHealthCheck = async () => {
    await new Promise((resolve) => setTimeout(resolve, 3_000));

    for (let attempt = 0; attempt < 60; attempt++) {
        try {
            const data = await $fetch<{ status: 'ok' }>('/api/healthz', { cache: 'no-store' });
            if (data.status === 'ok') return finish(true);
        } catch {
            //
        }

        await new Promise((resolve) => setTimeout(resolve, 2_000));
    }

    finish(false);
};

const handleUpdate = async () => {
    const confirmed = await askConfirmation({
        title: `Update to v${status.latestVersion}?`,
        description: 'Fileship will restart during the update.',
        confirmText: 'Update',
    });

    if (!confirmed) return;

    isUpdating.value = true;
    state.value = 'Pulling';

    let isPolling = false;

    try {
        const response = await $fetch('/api/updater/update', {
            method: 'POST',
            responseType: 'stream',
        });

        await parseSseStream<{ status?: string }>(response, (data) => {
            if (!data.status) return;

            state.value = data.status;

            if (data.status === 'Recreating' && !isPolling) {
                isPolling = true;
                pollHealthCheck();
            } else if (data.status === 'Success') finish(true);
            else if (data.status === 'Error') finish(false);
        });
    } catch (error: any) {
        if (state.value === 'Recreating' && !isPolling) {
            isPolling = true;
            pollHealthCheck();
        } else if (!isPolling) {
            $toast.error(
                error?.statusMessage || 'An unknown error occurred while updating Fileship',
            );
            isUpdating.value = false;
        }
    }
};
</script>
