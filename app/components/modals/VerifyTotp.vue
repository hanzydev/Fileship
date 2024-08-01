<template>
    <UiModal
        v-model="isOpen"
        flex="~ col items-center justify-center gap8"
        p8
        text-center
    >
        <div flex="~ items-center gap4">
            <Icon name="heroicons-solid:hand" size="96" />
            <h3>Verify TOTP</h3>
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

const props = defineProps<{ error?: string; disabled?: boolean }>();
const { error, disabled } = toRefs(props);

defineEmits<{
    got: [totp: string];
}>();
</script>
