<template>
    <UiAlertDialog :open="!!confirmState" @update:open="!$event && handleClose(false)">
        <UiAlertDialogContent v-if="confirmState" size="sm">
            <UiAlertDialogHeader>
                <UiAlertDialogTitle>{{ confirmState.title }}</UiAlertDialogTitle>
                <UiAlertDialogDescription v-if="confirmState.description">
                    {{ confirmState.description }}
                </UiAlertDialogDescription>
            </UiAlertDialogHeader>
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
</script>
