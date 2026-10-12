<template>
    <UiDialog :open="!!confirmState" @update:open="handleOpenChange">
        <UiDialogContent v-if="options" class="sm:max-w-sm" :show-close-button="false">
            <UiDialogHeader>
                <UiDialogTitle>{{ options.title }}</UiDialogTitle>
                <UiDialogDescription v-if="options.description">
                    {{ options.description }}
                </UiDialogDescription>
            </UiDialogHeader>
            <UiField v-if="options.option" orientation="horizontal">
                <UiCheckbox
                    id="confirm-option"
                    v-model="options.option.value"
                    :disabled="isRunning"
                />
                <UiFieldLabel for="confirm-option">{{ options.option.label }}</UiFieldLabel>
            </UiField>
            <UiDialogFooter>
                <UiButton variant="outline" :disabled="isRunning" @click="handleClose(false)">
                    Cancel
                </UiButton>
                <UiButton
                    :variant="options.destructive ? 'destructive' : 'default'"
                    :disabled="isRunning"
                    @click="handleConfirm"
                >
                    <UiSpinner v-if="isRunning" />
                    {{ options.confirmText || 'Continue' }}
                </UiButton>
            </UiDialogFooter>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
const confirmState = useConfirm();

const options = ref(confirmState.value);
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

watch(confirmState, (value) => value && (options.value = value));
</script>
