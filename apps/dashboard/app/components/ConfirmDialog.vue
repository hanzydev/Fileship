<template>
    <UiAlertDialog :open="!!confirmState" @update:open="handleOpenChange">
        <UiAlertDialogContent v-if="confirmState" size="sm">
            <UiAlertDialogHeader>
                <UiAlertDialogTitle>{{ confirmState.title }}</UiAlertDialogTitle>
                <UiAlertDialogDescription v-if="confirmState.description">
                    {{ confirmState.description }}
                </UiAlertDialogDescription>
            </UiAlertDialogHeader>
            <UiField v-if="confirmState.option" orientation="horizontal">
                <UiCheckbox id="confirm-option" v-model="confirmState.option.value" />
                <UiFieldLabel for="confirm-option">{{ confirmState.option.label }}</UiFieldLabel>
            </UiField>
            <UiAlertDialogFooter>
                <UiAlertDialogCancel @click="handleClose(false)">Cancel</UiAlertDialogCancel>
                <UiAlertDialogAction
                    :variant="confirmState.destructive ? 'destructive' : 'default'"
                    @click="handleClose(true)"
                >
                    {{ confirmState.confirmText || 'Continue' }}
                </UiAlertDialogAction>
            </UiAlertDialogFooter>
        </UiAlertDialogContent>
    </UiAlertDialog>
</template>

<script setup lang="ts">
const confirmState = useConfirm();

const handleClose = (value: boolean) => {
    confirmState.value?.resolve(value);
    confirmState.value = null;
};

const handleOpenChange = (open: boolean) => {
    if (!open) setTimeout(() => handleClose(false));
};
</script>
