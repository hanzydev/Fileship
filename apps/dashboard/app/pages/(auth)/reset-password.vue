<template>
    <div>
        <Head>
            <Title>{{ $t('auth.resetPassword.title') }}</Title>
        </Head>
        <Centered>
            <UiCard class="w-full max-w-sm">
                <template v-if="!token && isSent">
                    <UiCardHeader>
                        <UiCardTitle class="flex items-center gap-2">
                            <IMailCheck class="size-4" />
                            {{ $t('auth.resetPassword.sentTitle') }}
                        </UiCardTitle>
                        <UiCardDescription>
                            {{ $t('auth.resetPassword.sentDescription') }}
                        </UiCardDescription>
                    </UiCardHeader>
                    <UiCardFooter>
                        <UiButton class="w-full" variant="outline" as-child>
                            <NuxtLink to="/login">
                                <IArrowLeft />
                                {{ $t('auth.resetPassword.backToLogin') }}
                            </NuxtLink>
                        </UiButton>
                    </UiCardFooter>
                </template>

                <template v-else-if="!token">
                    <UiCardHeader>
                        <UiCardTitle>{{ $t('auth.resetPassword.heading') }}</UiCardTitle>
                        <UiCardDescription>
                            {{ $t('auth.resetPassword.description') }}
                        </UiCardDescription>
                    </UiCardHeader>
                    <UiCardContent>
                        <form @submit.prevent="handleRequest">
                            <UiFieldGroup>
                                <UiField>
                                    <UiFieldLabel for="login">
                                        {{ $t('auth.resetPassword.login') }}
                                    </UiFieldLabel>
                                    <UiInput
                                        id="login"
                                        v-model="form.login"
                                        autocomplete="username"
                                        autofocus
                                    />
                                    <UiFieldError
                                        v-if="formErrors?.login?._errors?.length"
                                        :errors="formErrors.login._errors"
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
                                    <UiButton type="submit" :disabled="isSubmitting || !form.login">
                                        <UiSpinner v-if="isSubmitting" />
                                        {{ $t('auth.resetPassword.submit') }}
                                    </UiButton>
                                    <UiFieldDescription class="text-center">
                                        <NuxtLink to="/login">
                                            {{ $t('auth.resetPassword.backToLogin') }}
                                        </NuxtLink>
                                    </UiFieldDescription>
                                </UiField>
                            </UiFieldGroup>
                        </form>
                    </UiCardContent>
                </template>

                <template v-else-if="linkError">
                    <UiCardHeader>
                        <UiCardTitle class="flex items-center gap-2">
                            <ICircleAlert class="size-4" />
                            {{ $t('auth.resetPassword.invalidTitle') }}
                        </UiCardTitle>
                        <UiCardDescription>{{ linkError }}</UiCardDescription>
                    </UiCardHeader>
                    <UiCardFooter>
                        <UiButton class="w-full" as-child>
                            <NuxtLink to="/reset-password">
                                {{ $t('auth.resetPassword.requestNew') }}
                            </NuxtLink>
                        </UiButton>
                    </UiCardFooter>
                </template>

                <template v-else-if="link">
                    <UiCardHeader>
                        <UiCardTitle>{{ $t('auth.resetPassword.newHeading') }}</UiCardTitle>
                        <UiCardDescription>
                            {{
                                $t('auth.resetPassword.newDescription', {
                                    username: link.username,
                                })
                            }}
                        </UiCardDescription>
                    </UiCardHeader>
                    <UiCardContent>
                        <form @submit.prevent="handleReset">
                            <UiFieldGroup>
                                <UiField>
                                    <UiFieldLabel for="password">
                                        {{ $t('auth.resetPassword.newPassword') }}
                                    </UiFieldLabel>
                                    <PasswordInput
                                        id="password"
                                        v-model="form.password"
                                        autocomplete="new-password"
                                        autofocus
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
                                <UiField v-if="link.totpRequired">
                                    <UiFieldLabel for="totp">
                                        {{ $t('auth.totp.code') }}
                                    </UiFieldLabel>
                                    <OtpInput id="totp" v-model="form.totp" />
                                    <UiFieldDescription>
                                        {{ $t('auth.resetPassword.totpDescription') }}
                                    </UiFieldDescription>
                                    <UiFieldError
                                        v-if="formErrors?.totp?._errors?.length"
                                        :errors="formErrors.totp._errors"
                                    />
                                </UiField>
                                <UiButton type="submit" :disabled="isSubmitting">
                                    <UiSpinner v-if="isSubmitting" />
                                    {{ $t('auth.resetPassword.save') }}
                                </UiButton>
                            </UiFieldGroup>
                        </form>
                    </UiCardContent>
                </template>

                <UiCardHeader v-else>
                    <UiCardTitle class="flex items-center gap-2">
                        <UiSpinner />
                        {{ $t('auth.resetPassword.newHeading') }}
                    </UiCardTitle>
                    <UiCardDescription>{{ $t('auth.resetPassword.checking') }}</UiCardDescription>
                </UiCardHeader>
            </UiCard>
        </Centered>
    </div>
</template>

<script setup lang="ts">
const route = useRoute();
const publicSettings = usePublicSettings();
const currentUser = useAuthUser();
const colorMode = useColorMode();
const { $toast } = useNuxtApp();
const { t } = useI18n();

const token = typeof route.query.token === 'string' ? route.query.token : '';

if (!token && !publicSettings.value.emailEnabled) {
    await navigateTo('/login', { replace: true });
}

const formErrors = ref();
const turnstileRef = ref();
const isSubmitting = ref(false);
const isSent = ref(false);
const linkError = ref('');

const form = reactive({
    login: '',
    turnstile: '',
    password: '',
    confirm: '',
    totp: '',
});

const { data: link } = await useAsyncData(
    'passwordResetLink',
    async () => {
        try {
            return await $fetch('/api/auth/password-reset', { query: { token } });
        } catch (error) {
            linkError.value = getErrorMessage(error);
            return null;
        }
    },
    { server: false, immediate: !!token },
);

const handleRequest = async () => {
    isSubmitting.value = true;
    formErrors.value = null;

    try {
        await $fetch('/api/auth/password-reset', {
            method: 'POST',
            body: { login: form.login, turnstile: form.turnstile || undefined },
        });

        isSent.value = true;
    } catch (error: any) {
        formErrors.value = error.data?.data?.formErrors;
        if (!formErrors.value) $toast.error(getErrorMessage(error));

        turnstileRef.value?.reset();
    }

    isSubmitting.value = false;
};

const handleReset = async () => {
    formErrors.value = null;

    if (form.password !== form.confirm) {
        formErrors.value = { confirm: { _errors: [t('auth.register.passwordMismatch')] } };
        return;
    }

    isSubmitting.value = true;

    try {
        await $fetch('/api/auth/password-reset/confirm', {
            method: 'POST',
            body: { token, password: form.password, totp: form.totp || undefined },
        });

        await navigateTo(currentUser.value ? '/dashboard' : '/login', { replace: true });
        $toast.success(t('auth.resetPassword.success'));
    } catch (error: any) {
        formErrors.value = error.data?.data?.formErrors;

        if (error.data?.data?.totpRequired) {
            formErrors.value = { totp: { _errors: [getErrorMessage(error)] } };
        } else if (!formErrors.value) {
            $toast.error(getErrorMessage(error));
        }
    }

    isSubmitting.value = false;
};
</script>
