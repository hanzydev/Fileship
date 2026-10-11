<template>
    <main class="flex flex-1 items-center justify-center p-4">
        <UiCard class="w-full max-w-sm">
            <form class="contents" @submit.prevent="emit('submit', password)">
                <UiCardHeader>
                    <UiCardTitle class="flex items-center gap-2">
                        <ILock class="size-4" />
                        {{ title }}
                    </UiCardTitle>
                    <UiCardDescription>Enter the password to continue.</UiCardDescription>
                </UiCardHeader>
                <UiCardContent>
                    <UiField :data-invalid="!!error">
                        <UiFieldLabel for="gate-password">Password</UiFieldLabel>
                        <UiInput
                            id="gate-password"
                            v-model="password"
                            type="password"
                            autofocus
                            autocomplete="off"
                        />
                        <UiFieldError v-if="error" :errors="[error]" />
                    </UiField>
                </UiCardContent>
                <UiCardFooter>
                    <UiButton type="submit" class="w-full" :disabled="!password || disabled">
                        <UiSpinner v-if="disabled" />
                        Continue
                    </UiButton>
                </UiCardFooter>
            </form>
        </UiCard>
    </main>
</template>

<script setup lang="ts">
const { title = 'Password protected' } = defineProps<{
    title?: string;
    error?: string;
    disabled?: boolean;
}>();

const emit = defineEmits<{
    submit: [password: string];
}>();

const password = ref('');
</script>
