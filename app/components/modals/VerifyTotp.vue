<template>
    <UiModal
        v-model="isOpen"
        flex="~ col items-center justify-center gap8"
        p8
        text-center
    >
        <div flex="~ lt-sm:col items-center gap4">
            <Icon name="heroicons-solid:hand" size="96" />
            <div lt-sm:space-y-2>
                <h3 sm:text-left>Two-Factor Authentication</h3>
                <p text-slate200>Enter the code from your authenticator app.</p>
            </div>
        </div>

        <UiTotpInput
            required
            :error
            :disabled
            @got="(totp) => $emit('got', totp)"
        />

        <UiButton
            alignment="center"
            variant="accent"
            icon="heroicons-solid:x"
            icon-size="24"
            wfull
            gap2
            @click="isOpen = false"
        >
            Cancel
        </UiButton>
    </UiModal>
</template>

<script setup lang="ts">
const isOpen = defineModel<boolean>({ required: false, default: true });

defineProps<{ error?: string; disabled?: boolean }>();

defineEmits<{
    got: [totp: string];
}>();
</script>
