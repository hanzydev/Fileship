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
                <UiCheckbox id="confirm-option" v-model="confirmState.option.value" />
                <UiFieldLabel for="confirm-option">{{ confirmState.option.label }}</UiFieldLabel>
            </UiField>
            <UiDialogFooter>
                <UiButton variant="outline" @click="handleClose(false)">Cancel</UiButton>
                <UiButton
                    :variant="confirmState.destructive ? 'destructive' : 'default'"
                    @click="handleClose(true)"
                >
                    {{ confirmState.confirmText || 'Continue' }}
                </UiButton>
            </UiDialogFooter>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
const confirmState = useConfirm();

const handleClose = (value: boolean) => {
    confirmState.value?.resolve(value);
    confirmState.value = null;
};

const handleOpenChange = (open: boolean) => {
    if (!open) handleClose(false);
};
</script>
