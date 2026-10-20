<template>
    <UiDialog :open="!!confirmState" @update:open="handleOpenChange">
        <UiDialogContent
            v-if="options"
            :key="openCount"
            class="sm:max-w-sm"
            :show-close-button="false"
        >
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
                    {{ $t('common.cancel') }}
                </UiButton>
                <UiButton
                    v-if="options.secondary"
                    :variant="options.secondary.destructive ? 'destructive' : 'outline'"
                    :disabled="isRunning"
                    @click="handleSecondary"
                >
                    <UiSpinner v-if="running === 'secondary'" />
                    {{ options.secondary.text }}
                </UiButton>
                <UiButton
                    :variant="options.destructive ? 'destructive' : 'default'"
                    :disabled="isRunning"
                    @click="handleConfirm"
                >
                    <UiSpinner v-if="running === 'confirm'" />
                    {{ options.confirmText || $t('common.continue') }}
                </UiButton>
            </UiDialogFooter>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
const confirmState = useConfirm();

const options = ref(confirmState.value);
const openCount = ref(0);
const running = ref<'confirm' | 'secondary' | null>(null);

const isRunning = computed(() => !!running.value);

const handleClose = (value: boolean) => {
    confirmState.value?.resolve(value);
    confirmState.value = null;
};

const runAction = async (kind: 'confirm' | 'secondary', action: () => Promise<unknown>) => {
    running.value = kind;

    const result = await action().catch(() => false);

    running.value = null;
    handleClose(result !== false);
};

const handleConfirm = () => {
    const { action } = confirmState.value!;
    if (!action) return handleClose(true);

    return runAction('confirm', action);
};

const handleSecondary = () => runAction('secondary', confirmState.value!.secondary!.action);

const handleOpenChange = (open: boolean) => {
    if (!open && !isRunning.value) handleClose(false);
};

watch(confirmState, (value, oldValue) => {
    if (!value) return;

    options.value = value;
    if (!oldValue) openCount.value++;
});
</script>
