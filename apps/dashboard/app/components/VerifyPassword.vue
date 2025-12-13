<template>
    <form
        rounded-xl
        bg-fs-overlay-1
        p8
        sm:w35rem
        space-y-10
        @submit.prevent="$emit('password', password)"
    >
        <div flex="~ gap4 items-center">
            <Icon name="heroicons:hand-raised-20-solid" size="48" />
            <h3>Verify password</h3>
        </div>

        <div mt10>
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
            <UiButton alignment="center" icon="lucide:x" icon-size="24" gap2 href="/login">
                Cancel
            </UiButton>
            <UiButton
                alignment="center"
                variant="accent"
                icon="lucide:check"
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
</template>

<script setup lang="ts">
const password = ref('');

defineProps<{ error?: string; disabled?: boolean }>();

defineEmits<{
    password: [password: string];
}>();
</script>
