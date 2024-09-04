<template>
    <UiModal v-model="isOpen" @closed="password = ''">
        <form
            flex="~ col justify-center gap8"
            p8
            text-center
            @submit.prevent="$emit('got', password)"
        >
            <div flex="~ items-center gap4">
                <Icon name="heroicons-solid:hand" size="96" />
                <h3>Verify your password</h3>
            </div>

            <div wfull>
                <UiInput
                    v-model="password"
                    type="password"
                    label="Password"
                    required
                    wfull
                    :error
                    :disabled
                />
            </div>

            <div grid="~ gap2 sm:cols-2 sm:gap4" wfull>
                <UiButton
                    alignment="center"
                    icon="heroicons-solid:x"
                    icon-size="24"
                    gap2
                    @click="
                        isOpen = false;
                        $emit('cancel');
                    "
                >
                    Cancel
                </UiButton>
                <UiButton
                    alignment="center"
                    variant="accent"
                    icon="heroicons-solid:check"
                    icon-size="24"
                    gap2
                    type="submit"
                    :loading="disabled"
                    :disabled
                >
                    Verify
                </UiButton>
            </div>
        </form>
    </UiModal>
</template>

<script setup lang="ts">
defineProps<{ error?: string; disabled?: boolean }>();

const isOpen = defineModel<boolean>({ required: false, default: true });
const password = ref('');

defineEmits<{
    got: [password: string];
    cancel: [];
}>();
</script>
