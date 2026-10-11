<template>
    <UiDialog :open="!!confirmState" @update:open="handleOpenChange">
        <UiDialogContent v-if="confirmState" class="sm:max-w-sm" :show-close-button="false">
            <UiDialogHeader>
                <UiDialogTitle>{{ confirmState.title }}</UiDialogTitle>
                <UiDialogDescription v-if="confirmState.description">
                    {{ confirmState.description }}
                </UiDialogDescription>
            </UiDialogHeader>
            <UiField v-if="confirmState.option" orientation="horizontal">
                <UiCheckbox
                    id="confirm-option"
                    v-model="confirmState.option.value"
                    :disabled="isRunning"
                />
                <UiFieldLabel for="confirm-option">{{ confirmState.option.label }}</UiFieldLabel>
            </UiField>
            <UiDialogFooter>
                <UiButton variant="outline" :disabled="isRunning" @click="handleClose(false)">
                    Cancel
                </UiButton>
                <UiButton
                    :variant="confirmState.destructive ? 'destructive' : 'default'"
                    :disabled="isRunning"
                    @click="handleConfirm"
                >
                    <UiSpinner v-if="isRunning" />
                    {{ confirmState.confirmText || 'Continue' }}
                </UiButton>
            </UiDialogFooter>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
const confirmState = useConfirm();

const isRunning = ref(false);

const handleClose = (value: boolean) => {
    confirmState.value?.resolve(value);
    confirmState.value = null;
};

const handleConfirm = async () => {
    const { action } = confirmState.value!;
    if (!action) return handleClose(true);

    isRunning.value = true;

    const result = await action().catch(() => false);

    isRunning.value = false;
    handleClose(result !== false);
};

const handleOpenChange = (open: boolean) => {
    if (!open && !isRunning.value) handleClose(false);
};
</script>
