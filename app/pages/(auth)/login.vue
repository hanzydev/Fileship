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
                text-start
                sm:w35rem
                :style="{
                    height: `calc(23.75rem${runtimeConfig.public.turnstile.siteKey ? ' + 7rem' : ''})`,
                }"
            >
                <Transition
                    enter-active-class="motion-safe:(animate-in fade-in slide-in-left-52)"
                    leave-active-class="motion-safe:(animate-out fade-out slide-out-left-52)"
                    @after-enter="(el) => el.querySelector('input')?.focus()"
                >
                    <form
                        v-if="section === 'login'"
                        absolute
                        hfull
                        wfull
                        rounded-lg
                        bg-fs-overlay-1
                        p8
                        flex="~ col justify-between"
                        @submit.prevent="handleSubmit()"
                    >
                        <div space-y-10>
                            <h2>Login</h2>
                            <div mt10 space-y-4>
                                <UiInput
                                    v-model="auth.username"
                                    label="Username"
                                    type="text"
                                    :error="formErrors?.username?._errors?.[0]"
                                    required
                                    wfull
                                    :disabled
                                />
                                <UiInput
                                    v-model="auth.password"
                                    label="Password"
                                    type="password"
                                    :error="formErrors?.password?._errors?.[0]"
                                    required
                                    wfull
                                    :disabled
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
                        </div>

                        <div flex="~ items-center gap4">
                            <UiButton
                                alignment="center"
                                wfull
                                gap2
                                variant="accent"
                                type="submit"
                                icon="heroicons-solid:lock-closed"
                                icon-size="20"
                                :loading="disabled"
                                :disabled
                            >
                                Login
                            </UiButton>
                            <ClientOnly>
                                <UiButton
                                    v-if="browserSupportsWebAuthn()"
                                    alignment="center"
                                    variant="secondary"
                                    max-w-fit
                                    wfull
                                    gap2
                                    type="submit"
                                    icon="heroicons-solid:finger-print"
                                    icon-size="20"
                                    :disabled
                                    @click="section = 'passkey'"
                                >
                                    Use passkey
                                </UiButton>
                            </ClientOnly>
                        </div>
                    </form>
                </Transition>
                <Transition
                    enter-active-class="motion-safe:(animate-in fade-in slide-in-right-52)"
                    leave-active-class="motion-safe:(animate-out fade-out slide-out-right-52)"
                    @after-enter="(el) => el.querySelector('input')?.focus()"
                >
                    <div
                        v-if="section === 'totp'"
                        absolute
                        hfull
                        wfull
                        rounded-lg
                        bg-fs-overlay-1
                        p8
                        flex="~ col justify-between"
                    >
                        <div space-y-10>
                            <div space-y-2>
                                <h2>Two-Factor Authentication</h2>
                                <p text-slate200>
                                    Enter the code from your authenticator app.
                                </p>
                            </div>
                            <div mt10 wfit>
                                <UiTotpInput
                                    type="text"
                                    :error
                                    required
                                    :disabled
                                    input-class="!wfull"
                                    @got="handleSubmit"
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
                        </div>

                        <div grid="~ cols-2 gap-4">
                            <UiButton
                                alignment="center"
                                wfull
                                gap2
                                icon="heroicons-solid:arrow-left"
                                icon-size="20"
                                :disabled
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
                                :loading="disabled"
                                :disabled
                            >
                                Login
                            </UiButton>
                        </div>
                    </div>
                </Transition>
                <Transition
                    enter-active-class="motion-safe:(animate-in fade-in slide-in-right-52)"
                    leave-active-class="motion-safe:(animate-out fade-out slide-out-right-52)"
                    @after-enter="(el) => el.querySelector('input')?.focus()"
                >
                    <form
                        v-if="section === 'passkey'"
                        absolute
                        hfull
                        wfull
                        rounded-lg
                        bg-fs-overlay-1
                        p8
                        flex="~ col justify-between"
                        @submit.prevent="handleSubmit()"
                    >
                        <div space-y-10>
                            <h2>Login with passkey</h2>
                            <div mt10>
                                <UiInput
                                    v-model="auth.username"
                                    label="Username"
                                    type="text"
                                    :error="formErrors?.username?._errors?.[0]"
                                    required
                                    wfull
                                    :disabled
                                />
                            </div>
                        </div>

                        <div grid="~ cols-2 gap-4">
                            <UiButton
                                alignment="center"
                                wfull
                                gap2
                                type="submit"
                                icon="heroicons-solid:arrow-left"
                                icon-size="20"
                                :disabled
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
                                icon="heroicons-solid:finger-print"
                                icon-size="20"
                                :loading="disabled"
                                :disabled
                            >
                                Login
                            </UiButton>
                        </div>
                    </form>
                </Transition>
            </div>
        </UiCentered>
    </div>
</template>

<script setup lang="ts">
import { toast } from 'vue-sonner';

import {
    browserSupportsWebAuthn,
    startAuthentication,
} from '@simplewebauthn/browser';
import type { PublicKeyCredentialRequestOptionsJSON } from '@simplewebauthn/types';

const formErrors = ref();
const turnstileRef = ref();

const error = ref<string>();
const disabled = ref(false);

const runtimeConfig = useRuntimeConfig();

const auth = reactive({
    username: '',
    password: '',
    turnstile: '',
});

const section = ref<'login' | 'totp' | 'passkey'>('login');

const route = useRoute();
const currentUser = useAuthUser();
const currentTheme = useTheme();

const handleUsePasskey = async () => {
    try {
        const optionsJSON = await $fetch<PublicKeyCredentialRequestOptionsJSON>(
            '/api/auth/login/passwordless',
            {
                method: 'POST',
                body: { username: auth.username, verify: false },
            },
        );

        const authenticationResponse = await startAuthentication({
            optionsJSON,
        });

        const { user, session } = await $fetch<any>(
            '/api/auth/login/passwordless',
            {
                method: 'POST',
                body: {
                    username: auth.username,
                    authenticationResponse,
                    expectedChallenge: optionsJSON.challenge,
                    verify: true,
                },
            },
        );

        currentUser.value = {
            ...user,
            currentSessionId: session.id,
            createdAt: new Date(user.createdAt),
        };

        currentTheme.value = user.theme as never;

        await navigateTo((route.query.redirectTo as string) || '/dashboard');

        toast.success('Logged in successfully with passkey');
    } catch (_error: any) {
        if (_error.data) {
            if (_error.data.message) toast.error(_error.data.message);
            else formErrors.value = _error.data.data;
        } else {
            toast.error('Failed to verify passkey');
        }
    }

    disabled.value = false;
};

const handleSubmit = async (totp?: string) => {
    disabled.value = true;
    formErrors.value = {};
    error.value = undefined;

    if (section.value === 'passkey') {
        return handleUsePasskey();
    }

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

        toast.success('Logged in successfully');
    } catch (_error: any) {
        console.log(_error);
        if (_error.data.message === 'Missing TOTP') {
            section.value = 'totp';
        } else {
            if (!_error.data.data) toast.error(_error.data.message);
            formErrors.value = _error.data.data;
        }

        turnstileRef.value?.reset();
    }

    disabled.value = false;
};

onMounted(() => {
    document.querySelector('input')?.focus();
});

definePageMeta({
    middleware: 'guest-only',
});
</script>
