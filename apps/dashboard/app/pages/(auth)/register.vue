<template>
    <div>
        <Head>
            <Title>Sign up</Title>
        </Head>
        <Centered>
            <UiCard class="w-full max-w-sm">
                <UiCardHeader>
                    <UiCardTitle>Create an account</UiCardTitle>
                    <UiCardDescription>
                        Enter your details below to create your account.
                    </UiCardDescription>
                </UiCardHeader>
                <UiCardContent>
                    <form @submit.prevent="handleSubmit">
                        <UiFieldGroup>
                            <UiField>
                                <UiFieldLabel for="username">Username</UiFieldLabel>
                                <UiInput
                                    id="username"
                                    v-model="form.username"
                                    type="text"
                                    placeholder="username"
                                    autocomplete="username"
                                    autofocus
                                />
                                <UiFieldError
                                    v-if="formErrors?.username?._errors?.length"
                                    :errors="formErrors.username._errors"
                                />
                            </UiField>
                            <UiField>
                                <UiFieldLabel for="password">Password</UiFieldLabel>
                                <PasswordInput
                                    id="password"
                                    v-model="form.password"
                                    autocomplete="new-password"
                                />
                                <UiFieldError
                                    v-if="formErrors?.password?._errors?.length"
                                    :errors="formErrors.password._errors"
                                />
                                <UiFieldDescription v-else>
                                    Must be at least 8 characters long.
                                </UiFieldDescription>
                            </UiField>
                            <UiField>
                                <UiFieldLabel for="confirm-password">Confirm password</UiFieldLabel>
                                <PasswordInput
                                    id="confirm-password"
                                    v-model="form.confirm"
                                    autocomplete="new-password"
                                />
                                <UiFieldError
                                    v-if="formErrors?.confirm?._errors?.length"
                                    :errors="formErrors.confirm._errors"
                                />
                            </UiField>
                            <NuxtTurnstile
                                v-if="runtimeConfig.public.turnstile.siteKey"
                                ref="turnstileRef"
                                v-model="form.turnstile"
                                :options="{ theme: colorMode.value as never, size: 'flexible' }"
                            />
                            <UiField>
                                <UiButton type="submit" :disabled="isRegistering">
                                    <UiSpinner v-if="isRegistering" />
                                    Create account
                                </UiButton>
                                <UiFieldDescription class="text-center">
                                    Already have an account?
                                    <NuxtLink to="/login">Login</NuxtLink>
                                </UiFieldDescription>
                            </UiField>
                        </UiFieldGroup>
                    </form>
                </UiCardContent>
            </UiCard>
        </Centered>
    </div>
</template>

<script setup lang="ts">
const formErrors = ref();
const turnstileRef = ref();
const isRegistering = ref(false);

const runtimeConfig = useRuntimeConfig();
const { $toast } = useNuxtApp();

const currentUser = useAuthUser();
const currentTheme = useTheme();
const colorMode = useColorMode();

if (!runtimeConfig.public.registrationEnabled) {
    await navigateTo('/login', { replace: true });
}

const form = reactive({
    username: '',
    password: '',
    confirm: '',
    turnstile: '',
});

const handleSubmit = async () => {
    formErrors.value = {};

    if (form.password !== form.confirm) {
        formErrors.value = { confirm: { _errors: ['Passwords do not match'] } };
        return;
    }

    isRegistering.value = true;

    try {
        const { user, session } = await $fetch('/api/auth/register', {
            method: 'POST',
            body: {
                username: form.username,
                password: form.password,
                turnstile: form.turnstile || undefined,
            },
        });

        currentUser.value = {
            ...user,
            currentSessionId: session.id,
            createdAt: new Date(user.createdAt),
        } as any;

        currentTheme.value = user.theme;
        useCookie('adminSessionId', { path: '/', sameSite: 'lax' }).value = null;

        await navigateTo('/dashboard');

        $toast.success('Account created');
    } catch (error: any) {
        formErrors.value = error.data?.data?.formErrors;
        if (!formErrors.value) $toast.error(getErrorMessage(error));

        turnstileRef.value?.reset();
    }

    isRegistering.value = false;
};

definePageMeta({
    middleware: 'guest-only',
});
</script>
