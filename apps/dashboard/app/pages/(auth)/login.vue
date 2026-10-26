<template>
    <div>
        <Head>
            <Title>{{ $t('auth.login.title') }}</Title>
        </Head>
        <Centered>
            <UiCard class="w-full max-w-sm">
                <template v-if="currentView === 'main'">
                    <UiCardHeader>
                        <UiCardTitle>{{ $t('auth.login.heading') }}</UiCardTitle>
                        <UiCardDescription>
                            {{
                                showPasswordForm
                                    ? $t('auth.login.description')
                                    : $t('auth.oidc.loginDescription', { provider })
                            }}
                        </UiCardDescription>
                    </UiCardHeader>
                    <UiCardContent>
                        <UiFieldGroup v-if="!showPasswordForm">
                            <UiField>
                                <UiButton as-child>
                                    <a :href="oidcLoginUrl" @click="isRedirecting = true">
                                        <UiSpinner v-if="isRedirecting" />
                                        <ILogIn v-else />
                                        {{ $t('auth.oidc.continue', { provider }) }}
                                    </a>
                                </UiButton>
                                <UiButton
                                    variant="ghost"
                                    type="button"
                                    @click="isAdminLogin = true"
                                >
                                    {{ $t('auth.oidc.adminLogin') }}
                                </UiButton>
                            </UiField>
                        </UiFieldGroup>
                        <form v-else @submit.prevent="handleSubmit">
                            <UiFieldGroup>
                                <UiField>
                                    <UiFieldLabel for="username">
                                        {{ $t('auth.username') }}
                                    </UiFieldLabel>
                                    <UiInput
                                        id="username"
                                        v-model="auth.username"
                                        type="text"
                                        :placeholder="$t('auth.usernamePlaceholder')"
                                        autocomplete="username webauthn"
                                        autofocus
                                    />
                                    <UiFieldError
                                        v-if="formErrors?.username?._errors?.length"
                                        :errors="formErrors.username._errors"
                                    />
                                </UiField>
                                <UiField>
                                    <div class="flex items-center">
                                        <UiFieldLabel for="password">
                                            {{ $t('auth.password') }}
                                        </UiFieldLabel>
                                        <NuxtLink
                                            v-if="publicSettings.emailEnabled"
                                            to="/reset-password"
                                            class="ml-auto text-sm underline-offset-4 hover:underline"
                                        >
                                            {{ $t('auth.login.forgotPassword') }}
                                        </NuxtLink>
                                    </div>
                                    <PasswordInput
                                        id="password"
                                        v-model="auth.password"
                                        placeholder="••••••••"
                                        autocomplete="current-password"
                                    />
                                    <UiFieldError
                                        v-if="formErrors?.password?._errors?.length"
                                        :errors="formErrors.password._errors"
                                    />
                                </UiField>
                                <NuxtTurnstile
                                    v-if="publicSettings.turnstileSiteKey"
                                    ref="turnstileRef"
                                    v-model="auth.turnstile"
                                    :site-key="publicSettings.turnstileSiteKey"
                                    :options="{ theme: colorMode.value as never, size: 'flexible' }"
                                />
                                <UiField>
                                    <UiButton type="submit" :disabled="loggingIn">
                                        <UiSpinner v-if="loggingIn" />
                                        {{ $t('auth.login.submit') }}
                                    </UiButton>
                                    <ClientOnly>
                                        <UiButton
                                            v-if="browserSupportsWebAuthn()"
                                            variant="outline"
                                            type="button"
                                            :disabled="loggingIn || passkeyLoggingIn"
                                            @click="handlePasskeyLogin"
                                        >
                                            <UiSpinner v-if="passkeyLoggingIn" />
                                            <IFingerprintPattern v-else />
                                            {{ $t('auth.login.passkey') }}
                                        </UiButton>
                                    </ClientOnly>
                                    <UiButton
                                        v-if="publicSettings.oidcLogin"
                                        variant="outline"
                                        as-child
                                    >
                                        <a :href="oidcLoginUrl" @click="isRedirecting = true">
                                            <UiSpinner v-if="isRedirecting" />
                                            <ILogIn v-else />
                                            {{ $t('auth.oidc.continue', { provider }) }}
                                        </a>
                                    </UiButton>
                                    <UiFieldDescription
                                        v-if="publicSettings.registrationEnabled"
                                        class="text-center"
                                    >
                                        {{ $t('auth.login.noAccount') }}
                                        <NuxtLink to="/register">
                                            {{ $t('auth.login.signUp') }}
                                        </NuxtLink>
                                    </UiFieldDescription>
                                </UiField>
                            </UiFieldGroup>
                        </form>
                    </UiCardContent>
                </template>

                <template v-else>
                    <UiCardHeader>
                        <UiCardTitle>{{ $t('auth.totp.title') }}</UiCardTitle>
                        <UiCardDescription>
                            {{ $t('auth.totp.description') }}
                        </UiCardDescription>
                    </UiCardHeader>
                    <UiCardContent>
                        <form @submit.prevent="handleSubmit">
                            <UiFieldGroup>
                                <UiField>
                                    <UiFieldLabel for="totp">
                                        {{ $t('auth.totp.code') }}
                                    </UiFieldLabel>
                                    <OtpInput id="totp" v-model="auth.totp" />
                                    <UiFieldError
                                        v-if="formErrors?.totp?._errors?.length"
                                        :errors="formErrors.totp._errors"
                                    />
                                </UiField>
                                <NuxtTurnstile
                                    v-if="publicSettings.turnstileSiteKey"
                                    ref="turnstileRef"
                                    v-model="auth.turnstile"
                                    :site-key="publicSettings.turnstileSiteKey"
                                    :options="{ theme: colorMode.value as never, size: 'flexible' }"
                                />
                                <UiField>
                                    <UiButton type="submit" :disabled="loggingIn">
                                        <UiSpinner v-if="loggingIn" />
                                        {{ $t('auth.totp.verify') }}
                                    </UiButton>
                                    <UiButton
                                        variant="outline"
                                        type="button"
                                        :disabled="loggingIn"
                                        @click="handleBack"
                                    >
                                        <IArrowLeft />
                                        {{ $t('auth.totp.back') }}
                                    </UiButton>
                                </UiField>
                            </UiFieldGroup>
                        </form>
                    </UiCardContent>
                </template>
            </UiCard>
        </Centered>
    </div>
</template>

<script setup lang="ts">
import {
    browserSupportsWebAuthn,
    type PublicKeyCredentialRequestOptionsJSON,
    sendSignal,
    startAuthentication,
} from '@simplewebauthn/browser';

const formErrors = ref();
const turnstileRef = ref();

const loggingIn = ref(false);
const passkeyLoggingIn = ref(false);
const isRedirecting = ref(false);
const isAdminLogin = ref(false);

const publicSettings = usePublicSettings();
const provider = useOidcProvider();
const { $toast } = useNuxtApp();
const { t } = useI18n();

const auth = reactive({
    username: '',
    password: '',
    turnstile: '',
    totp: '',
});

const currentView = ref<'main' | 'totp'>('main');

const route = useRoute();
const currentUser = useAuthUser();
const currentTheme = useTheme();
const colorMode = useColorMode();

const showPasswordForm = computed(
    () =>
        publicSettings.value.passwordLogin || !publicSettings.value.oidcLogin || isAdminLogin.value,
);

const oidcLoginUrl = computed(() => {
    const redirectTo = route.query.redirectTo;

    return typeof redirectTo === 'string' && /^\/(?![/\\])/.test(redirectTo)
        ? `/api/auth/oidc/login?${new URLSearchParams({ redirectTo })}`
        : '/api/auth/oidc/login';
});

const handleLoggedIn = async (user: any, session: any, messageKey: string) => {
    currentUser.value = {
        ...user,
        currentSessionId: session.id,
        createdAt: new Date(user.createdAt),
    };

    currentTheme.value = user.theme;
    await applyLanguage(user.language);

    const redirectTo = route.query.redirectTo;

    await navigateTo(
        typeof redirectTo === 'string' && /^\/(?![/\\])/.test(redirectTo)
            ? redirectTo
            : '/dashboard',
    );

    $toast.success(t(messageKey));
};

const handleSubmit = async () => {
    loggingIn.value = true;
    formErrors.value = {};

    try {
        const { user, session } = await $fetch('/api/auth/login', {
            method: 'POST',
            body: { ...auth, totp: auth.totp || undefined },
        });

        await handleLoggedIn(user, session, 'auth.login.success');
    } catch (error: any) {
        if (error.data?.data?.totpRequired) {
            currentView.value = 'totp';
            await nextTick();
            document.querySelector<HTMLInputElement>('#totp input, input#totp')?.focus();
        } else {
            if (!error.data?.data) $toast.error(getErrorMessage(error));
            formErrors.value = error.data?.data?.formErrors;
        }

        turnstileRef.value?.reset();
    }

    loggingIn.value = false;
};

const handleBack = () => {
    auth.totp = '';
    formErrors.value = {};
    currentView.value = 'main';
};

const handlePasskeyLogin = async () => {
    passkeyLoggingIn.value = true;

    try {
        const { ticket, authenticationOptions } = await $fetch<{
            ticket: string;
            authenticationOptions: PublicKeyCredentialRequestOptionsJSON;
        }>('/api/auth/login/passwordless', {
            method: 'POST',
            body: { verify: false },
        });

        const authenticationResponse = await startAuthentication({
            optionsJSON: authenticationOptions,
        }).catch(() => null);

        if (authenticationResponse) {
            try {
                const { user, session } = (await $fetch('/api/auth/login/passwordless', {
                    method: 'POST',
                    body: { verify: true, ticket, authenticationResponse },
                })) as any;

                await handleLoggedIn(user, session, 'auth.login.passkeySuccess');
            } catch (error: any) {
                if (error.data?.data?.unknownCredential) {
                    await sendSignal({
                        signalName: 'unknownCredential',
                        rpID: location.hostname,
                        credentialID: authenticationResponse.id,
                    }).catch(() => null);
                }

                $toast.error(getErrorMessage(error));
            }
        }
    } catch (error) {
        $toast.error(getErrorMessage(error));
    }

    passkeyLoggingIn.value = false;
};

onMounted(() => {
    const status = route.query.sso;
    if (typeof status !== 'string') return;

    $toast.error(t(oidcErrorKey(status), { provider: provider.value }));
    navigateTo({ query: { ...route.query, sso: undefined } }, { replace: true });
});

definePageMeta({
    middleware: 'guest-only',
});
</script>
