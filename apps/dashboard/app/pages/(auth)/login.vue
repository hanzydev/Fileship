<template>
    <div>
        <Head>
            <Title>Login</Title>
        </Head>
        <UiCentered>
            <div
                relative
                w20rem
                overflow-hidden
                rounded-xl
                bg-fs-overlay-1
                text-start
                transition-height
                duration-250
                sm:w35rem
                :style="{ height: `${height}px` }"
            >
                <Transition
                    enter-active-class="motion-safe:(animate-in fade-in data-[hubmode=true]:slide-in-from-left data-[hubmode=false]:slide-in-from-right animate-duration-250)"
                    leave-active-class="motion-safe:(animate-out fade-out data-[hubmode=true]:slide-out-to-left data-[hubmode=false]:slide-out-to-right animate-duration-250)"
                    :data-hubmode="section === 'login'"
                    @enter="calculateHeight"
                    @after-enter="$event.querySelector('input')?.focus()"
                >
                    <form
                        v-if="section === 'login'"
                        absolute
                        wfull
                        p8
                        flex="~ col justify-between gap10"
                        @submit.prevent="handleSubmit()"
                    >
                        <h2>Login</h2>

                        <div space-y-4>
                            <UiInput
                                v-model="auth.username"
                                label="Username"
                                type="text"
                                :error="formErrors?.username?._errors?.[0]"
                                required
                                wfull
                                :disabled="loggingIn"
                            />
                            <UiInput
                                v-model="auth.password"
                                label="Password"
                                type="password"
                                :error="formErrors?.password?._errors?.[0]"
                                required
                                wfull
                                :disabled="loggingIn"
                            />
                        </div>

                        <NuxtTurnstile
                            v-if="runtimeConfig.public.turnstile.siteKey"
                            ref="turnstileRef"
                            v-model="auth.turnstile"
                            :options="{
                                theme: 'dark',
                                size: 'flexible',
                            }"
                        />

                        <div flex="~ items-center gap-3">
                            <UiButton
                                alignment="center"
                                wfull
                                flex-1
                                gap2
                                variant="accent"
                                type="submit"
                                icon="heroicons-solid:lock-closed"
                                icon-size="20"
                                :loading="loggingIn"
                                :disabled="passkeyLoggingIn || loggingIn"
                            >
                                Login
                            </UiButton>
                            <ClientOnly>
                                <UiButton
                                    v-if="browserSupportsWebAuthn()"
                                    alignment="center"
                                    size-10
                                    gap2
                                    variant="secondary"
                                    icon="heroicons-solid:finger-print"
                                    icon-size="24"
                                    p0="!"
                                    :loading="passkeyLoggingIn"
                                    :disabled="loggingIn || passkeyLoggingIn"
                                    @click="handlePasskeyLogin"
                                />
                            </ClientOnly>
                        </div>
                    </form>
                    <div
                        v-else-if="section === 'totp'"
                        absolute
                        wfull
                        p8
                        flex="~ col justify-between gap10"
                    >
                        <div space-y-2>
                            <h2>Multi-Factor Authentication</h2>
                            <p text-fs-muted-1>Enter the code from your authenticator app.</p>
                        </div>

                        <UiTotpInput
                            type="text"
                            :error
                            required
                            :disabled="loggingIn"
                            input-class="!wfull"
                            @got="handleSubmit"
                        />

                        <NuxtTurnstile
                            v-if="runtimeConfig.public.turnstile.siteKey"
                            ref="turnstileRef"
                            v-model="auth.turnstile"
                            :options="{
                                theme: 'dark',
                                size: 'flexible',
                            }"
                        />

                        <div grid="~ cols-2 gap-4">
                            <UiButton
                                alignment="center"
                                wfull
                                gap2
                                variant="secondary"
                                icon="heroicons-solid:arrow-left"
                                icon-size="20"
                                :disabled="loggingIn"
                                @click="section = 'login'"
                            >
                                Back
                            </UiButton>
                            <UiButton
                                alignment="center"
                                wfull
                                gap2
                                variant="accent"
                                type="submit"
                                icon="heroicons-solid:lock-closed"
                                icon-size="20"
                                :loading="loggingIn"
                                :disabled="loggingIn"
                            >
                                Login
                            </UiButton>
                        </div>
                    </div>
                </Transition>
            </div>
        </UiCentered>
    </div>
</template>

<script setup lang="ts">
import { browserSupportsWebAuthn, startAuthentication } from '@simplewebauthn/browser';
import type { PublicKeyCredentialRequestOptionsJSON } from '@simplewebauthn/types';

const formErrors = ref();
const turnstileRef = ref();

const error = ref<string>();
const loggingIn = ref(false);
const passkeyLoggingIn = ref(false);

const height = ref(380 /** initial */);

const runtimeConfig = useRuntimeConfig();
const { $toast } = useNuxtApp();

const auth = reactive({
    username: '',
    password: '',
    turnstile: '',
});

const section = ref<'login' | 'totp'>('login');

const route = useRoute();
const currentUser = useAuthUser();
const currentTheme = useTheme();

const handleSubmit = async (totp?: string) => {
    loggingIn.value = true;
    formErrors.value = {};
    error.value = undefined;

    try {
        const { user, session } = await $fetch('/api/auth/login', {
            method: 'POST',
            body: {
                ...auth,
                totp,
            },
        });

        currentUser.value = {
            ...user,
            currentSessionId: session.id,
            createdAt: new Date(user.createdAt),
        };

        currentTheme.value = user.theme as never;

        await navigateTo((route.query.redirectTo as string) || '/dashboard');

        $toast.success('Logged in successfully');
    } catch (_error: any) {
        if (_error.data.message === 'Missing TOTP') {
            section.value = 'totp';
        } else {
            if (!_error.data.data) $toast.error(_error.data.message);
            formErrors.value = _error.data.data?.formErrors;
        }

        turnstileRef.value?.reset();
    }

    loggingIn.value = false;
};

const handlePasskeyLogin = async () => {
    passkeyLoggingIn.value = true;

    const optionsJSON = await $fetch<PublicKeyCredentialRequestOptionsJSON>(
        '/api/auth/login/passwordless',
        {
            method: 'POST',
            body: { verify: false },
            credentials: 'include',
        },
    );

    const authenticationResponse = await startAuthentication({
        optionsJSON,
    }).catch(() => null);

    if (authenticationResponse) {
        try {
            const { user, session } = await $fetch<any>('/api/auth/login/passwordless', {
                method: 'POST',
                body: {
                    authenticationResponse,
                    expectedChallenge: optionsJSON.challenge,
                    verify: true,
                },
            });

            currentUser.value = {
                ...user,
                currentSessionId: session.id,
                createdAt: new Date(user.createdAt),
            };

            currentTheme.value = user.theme as never;

            await navigateTo((route.query.redirectTo as string) || '/dashboard');

            $toast.success('Logged in successfully with passkey');
        } catch (_error: any) {
            if (_error.data) {
                if (_error.data.message) $toast.error(_error.data.message);
                else formErrors.value = _error.data.data;
            } else {
                $toast.error('Failed to verify passkey');
            }
        }
    }

    passkeyLoggingIn.value = false;
};

const calculateHeight = (el: Element) => {
    height.value = el.clientHeight + (runtimeConfig.public.turnstile.siteKey ? 71 : 0);
};

onMounted(async () => {
    const container = document.querySelector('form');

    if (container) {
        calculateHeight(container);
        container.querySelector('input')?.focus();
    }
});

definePageMeta({
    middleware: 'guest-only',
});
</script>
