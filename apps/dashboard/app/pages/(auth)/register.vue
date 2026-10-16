<template>
    <div>
        <Head>
            <Title>{{ $t('auth.register.title') }}</Title>
        </Head>
        <Centered>
            <UiCard class="w-full max-w-sm">
                <UiCardHeader>
                    <UiCardTitle>{{ $t('auth.register.heading') }}</UiCardTitle>
                    <UiCardDescription>
                        {{ $t('auth.register.description') }}
                    </UiCardDescription>
                </UiCardHeader>
                <UiCardContent>
                    <form @submit.prevent="handleSubmit">
                        <UiFieldGroup>
                            <UiField>
                                <UiFieldLabel for="username">
                                    {{ $t('auth.username') }}
                                </UiFieldLabel>
                                <UiInput
                                    id="username"
                                    v-model="form.username"
                                    type="text"
                                    :placeholder="$t('auth.usernamePlaceholder')"
                                    autocomplete="username"
                                    autofocus
                                />
                                <UiFieldError
                                    v-if="formErrors?.username?._errors?.length"
                                    :errors="formErrors.username._errors"
                                />
                            </UiField>
                            <UiField>
                                <UiFieldLabel for="password">
                                    {{ $t('auth.password') }}
                                </UiFieldLabel>
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
                                    {{ $t('auth.register.passwordHint') }}
                                </UiFieldDescription>
                            </UiField>
                            <UiField>
                                <UiFieldLabel for="confirm-password">
                                    {{ $t('auth.register.confirmPassword') }}
                                </UiFieldLabel>
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
                                v-if="publicSettings.turnstileSiteKey"
                                ref="turnstileRef"
                                v-model="form.turnstile"
                                :site-key="publicSettings.turnstileSiteKey"
                                :options="{ theme: colorMode.value as never, size: 'flexible' }"
                            />
                            <UiField>
                                <UiButton type="submit" :disabled="isRegistering">
                                    <UiSpinner v-if="isRegistering" />
                                    {{ $t('auth.register.submit') }}
                                </UiButton>
                                <UiFieldDescription class="text-center">
                                    {{ $t('auth.register.haveAccount') }}
                                    <NuxtLink to="/login">{{ $t('auth.register.logIn') }}</NuxtLink>
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

const publicSettings = usePublicSettings();
const { $toast } = useNuxtApp();
const { t } = useI18n();

const currentUser = useAuthUser();
const currentTheme = useTheme();
const colorMode = useColorMode();

if (!publicSettings.value.registrationEnabled) {
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
        formErrors.value = { confirm: { _errors: [t('auth.register.passwordMismatch')] } };
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

        await navigateTo('/dashboard');

        $toast.success(t('auth.register.success'));
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
