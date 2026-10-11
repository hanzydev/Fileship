<template>
    <div>
        <Head>
            <Title>Login</Title>
        </Head>
        <Centered>
            <UiCard class="w-full max-w-sm">
                <template v-if="currentView === 'main'">
                    <UiCardHeader>
                        <UiCardTitle>Login to your account</UiCardTitle>
                        <UiCardDescription>
                            Enter your username below to login to your account.
                        </UiCardDescription>
                    </UiCardHeader>
                    <UiCardContent>
                        <form @submit.prevent="handleSubmit">
                            <UiFieldGroup>
                                <UiField>
                                    <UiFieldLabel for="username">Username</UiFieldLabel>
                                    <UiInput
                                        id="username"
                                        v-model="auth.username"
                                        type="text"
                                        placeholder="username"
                                        autocomplete="username webauthn"
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
                                    v-if="runtimeConfig.public.turnstile.siteKey"
                                    ref="turnstileRef"
                                    v-model="auth.turnstile"
                                    :options="{ theme: colorMode.value as never, size: 'flexible' }"
                                />
                                <UiField>
                                    <UiButton type="submit" :disabled="loggingIn">
                                        <UiSpinner v-if="loggingIn" />
                                        Login
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
                                            Login with passkey
                                        </UiButton>
                                    </ClientOnly>
                                </UiField>
                            </UiFieldGroup>
                        </form>
                    </UiCardContent>
                </template>

                <template v-else>
                    <UiCardHeader>
                        <UiCardTitle>Two-factor authentication</UiCardTitle>
                        <UiCardDescription>
                            Enter the 6-digit code from your authenticator app to continue.
                        </UiCardDescription>
                    </UiCardHeader>
                    <UiCardContent>
                        <form @submit.prevent="handleSubmit">
                            <UiFieldGroup>
                                <UiField>
                                    <UiFieldLabel for="totp">Authenticator code</UiFieldLabel>
                                    <OtpInput id="totp" v-model="auth.totp" />
                                    <UiFieldError
                                        v-if="formErrors?.totp?._errors?.length"
                                        :errors="formErrors.totp._errors"
                                    />
                                </UiField>
                                <NuxtTurnstile
                                    v-if="runtimeConfig.public.turnstile.siteKey"
                                    ref="turnstileRef"
                                    v-model="auth.turnstile"
                                    :options="{ theme: colorMode.value as never, size: 'flexible' }"
                                />
                                <UiField>
                                    <UiButton type="submit" :disabled="loggingIn">
                                        <UiSpinner v-if="loggingIn" />
                                        Verify
                                    </UiButton>
                                    <UiButton
                                        variant="outline"
                                        type="button"
                                        :disabled="loggingIn"
                                        @click="handleBack"
                                    >
                                        <IArrowLeft />
                                        Back
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

const runtimeConfig = useRuntimeConfig();
const { $toast } = useNuxtApp();

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

const handleLoggedIn = async (user: any, session: any, message: string) => {
    currentUser.value = {
        ...user,
        currentSessionId: session.id,
        createdAt: new Date(user.createdAt),
    };

    currentTheme.value = user.theme;
    useCookie('adminSessionId', { path: '/', sameSite: 'lax' }).value = null;

    await navigateTo((route.query.redirectTo as string) || '/dashboard');

    $toast.success(message);
};

const handleSubmit = async () => {
    loggingIn.value = true;
    formErrors.value = {};

    try {
        const { user, session } = await $fetch('/api/auth/login', {
            method: 'POST',
            body: { ...auth, totp: auth.totp || undefined },
        });

        await handleLoggedIn(user, session, 'Logged in successfully');
    } catch (error: any) {
        if (getErrorMessage(error) === 'Missing TOTP') {
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

                await handleLoggedIn(user, session, 'Logged in successfully with passkey');
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

definePageMeta({
    middleware: 'guest-only',
});
</script>
