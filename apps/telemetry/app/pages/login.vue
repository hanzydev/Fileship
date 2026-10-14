<template>
    <div>
        <Head>
            <Title>Login</Title>
        </Head>
        <Centered>
            <UiCard class="w-full max-w-sm">
                <UiCardHeader>
                    <UiCardTitle>Fileship Telemetry</UiCardTitle>
                    <UiCardDescription>Enter the panel password to continue.</UiCardDescription>
                </UiCardHeader>
                <UiCardContent>
                    <form @submit.prevent="handleSubmit">
                        <UiFieldGroup>
                            <UiField>
                                <UiFieldLabel for="password">Password</UiFieldLabel>
                                <PasswordInput
                                    id="password"
                                    v-model="password"
                                    placeholder="••••••••"
                                    autocomplete="current-password"
                                    autofocus
                                />
                                <UiFieldError
                                    v-if="formErrors?.password?._errors?.length"
                                    :errors="formErrors.password._errors"
                                />
                            </UiField>
                            <UiField>
                                <UiButton type="submit" :disabled="loggingIn">
                                    <UiSpinner v-if="loggingIn" />
                                    Login
                                </UiButton>
                            </UiField>
                        </UiFieldGroup>
                    </form>
                </UiCardContent>
            </UiCard>
        </Centered>
    </div>
</template>

<script setup lang="ts">
const admin = useAdmin();
const route = useRoute();
const { $toast } = useNuxtApp();

const password = ref('');
const formErrors = ref();
const loggingIn = ref(false);

const handleSubmit = async () => {
    loggingIn.value = true;
    formErrors.value = null;

    try {
        await $fetch('/api/auth/login', {
            method: 'POST',
            body: { password: password.value },
        });

        admin.value = true;

        const redirectTo = route.query.redirectTo as string | undefined;
        await navigateTo(
            redirectTo?.startsWith('/') && !redirectTo.startsWith('//') ? redirectTo : '/overview',
        );
    } catch (error: any) {
        formErrors.value = error.data?.data?.formErrors;
        if (!formErrors.value) $toast.error(getErrorMessage(error));
    }

    loggingIn.value = false;
};
</script>
