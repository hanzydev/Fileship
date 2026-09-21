<template>
    <div>
        <Head>
            <Title>Login</Title>
        </Head>
        <Centered>
            <UiCard
                class="p-0! relative overflow-hidden duration-250 transition-[height] w-full max-w-sm"
                :style="{ height: `${height}px` }"
            >
                <form @submit.prevent="handleSubmit">
                    <Transition
                        enter-active-class="animate-in fade-in data-[view=main]:slide-in-from-left data-[view=totp]:slide-in-from-right duration-250"
                        leave-active-class="animate-out fade-out data-[view=main]:slide-out-to-left data-[view=totp]:slide-out-to-right duration-250"
                        @enter="calculateHeight"
                        @after-enter="(event) => event.querySelector('input')?.focus()"
                    >
                        <div
                            v-if="currentView === 'main'"
                            data-view="main"
                            class="text-left py-6 flex flex-col gap-6 absolute w-full"
                        >
                            <UiCardHeader>
                                <UiCardTitle>Login to your account</UiCardTitle>
                                <UiCardDescription>
                                    Enter your username below to login to your account.
                                </UiCardDescription>
                            </UiCardHeader>
                            <UiCardContent>
                                <div class="flex flex-col gap-4">
                                    <UiField>
                                        <UiFieldLabel for="username">Username</UiFieldLabel>
                                        <UiInput
                                            id="username"
                                            v-model="auth.username"
                                            type="text"
                                            placeholder="hanzydev"
                                        />
                                        <UiFieldError
                                            v-if="formErrors?.username?._errors?.length"
                                            :errors="formErrors?.username?._errors"
                                        />
                                    </UiField>
                                    <UiField>
                                        <UiFieldLabel for="password">Password</UiFieldLabel>
                                        <UiInput
                                            id="password"
                                            v-model="auth.password"
                                            type="password"
                                            placeholder="••••••••"
                                        />
                                        <UiFieldError
                                            v-if="formErrors?.password?._errors?.length"
                                            :errors="formErrors?.password?._errors"
                                        />
                                    </UiField>
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
                            </UiCardContent>
                            <UiCardFooter class="flex flex-col gap-2">
                                <UiButtonGroup class="w-full">
                                    <UiButton class="flex-1" type="submit" :disabled="loggingIn">
                                        <UiSpinner v-if="loggingIn" class="size-4.5" />
                                        <template v-else>Login</template>
                                    </UiButton>
                                    <ClientOnly>
                                        <UiButton
                                            v-if="browserSupportsWebAuthn()"
                                            variant="secondary"
                                            type="button"
                                            :disabled="loggingIn || passkeyLoggingIn"
                                            @click="handlePasskeyLogin"
                                        >
                                            <UiSpinner v-if="passkeyLoggingIn" class="size-4.5" />
                                            <IFingerprintPattern v-else :size="18" />
                                        </UiButton>
                                    </ClientOnly>
                                </UiButtonGroup>
                            </UiCardFooter>
                        </div>
                        <div
                            v-else-if="currentView === 'totp'"
                            data-view="totp"
                            class="text-left py-6 flex flex-col gap-6 absolute w-full"
                        >
                            <UiCardHeader>
                                <UiCardTitle>Multi-Factor Authentication</UiCardTitle>
                                <UiCardDescription>
                                    Enter the 6-digit code from your authenticator app to continue.
                                </UiCardDescription>
                            </UiCardHeader>
                            <UiCardContent>
                                <div class="flex flex-col gap-4">
                                    <UiField>
                                        <UiFieldLabel for="totp">TOTP Code</UiFieldLabel>
                                        <UiInputOTP
                                            id="totp"
                                            v-model="auth.totp"
                                            class="w-full"
                                            :maxlength="6"
                                        >
                                            <UiInputOTPGroup class="flex-1">
                                                <UiInputOTPSlot :index="0" class="flex-1 w-full" />
                                                <UiInputOTPSlot :index="1" class="flex-1 w-full" />
                                                <UiInputOTPSlot :index="2" class="flex-1 w-full" />
                                            </UiInputOTPGroup>

                                            <UiInputOTPSeparator />

                                            <UiInputOTPGroup class="flex-1">
                                                <UiInputOTPSlot :index="3" class="flex-1 w-full" />
                                                <UiInputOTPSlot :index="4" class="flex-1 w-full" />
                                                <UiInputOTPSlot :index="5" class="flex-1 w-full" />
                                            </UiInputOTPGroup>
                                        </UiInputOTP>
                                        <UiFieldError
                                            v-if="formErrors?.totp?._errors?.length"
                                            :errors="formErrors?.totp?._errors"
                                        />
                                    </UiField>

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
                            </UiCardContent>
                            <UiCardFooter class="flex flex-col gap-2">
                                <UiButtonGroup class="w-full">
                                    <UiButton
                                        variant="secondary"
                                        type="button"
                                        :disabled="loggingIn"
                                        @click="
                                            auth.totp = undefined;
                                            formErrors = {};
                                            currentView = 'main';
                                        "
                                    >
                                        <IArrowLeft :size="18" />
                                    </UiButton>
                                    <UiButton class="flex-1" type="submit" :disabled="loggingIn">
                                        <UiSpinner v-if="loggingIn" class="size-4.5" />
                                        <template v-else>Login</template>
                                    </UiButton>
                                </UiButtonGroup>
                            </UiCardFooter>
                        </div>
                    </Transition>
                </form>
            </UiCard>
        </Centered>
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

const height = ref(333 /** initial */);

const runtimeConfig = useRuntimeConfig();
const { $toast } = useNuxtApp();

const auth = reactive({
    username: '',
    password: '',
    turnstile: '',
    totp: undefined as string | undefined,
});

const currentView = ref<'main' | 'totp'>('main');

const route = useRoute();
const currentUser = useAuthUser();
const currentTheme = useTheme();

const handleSubmit = async () => {
    loggingIn.value = true;
    formErrors.value = {};
    error.value = undefined;
    await nextTick();
    calculateHeight();

    try {
        const { user, session } = await $fetch('/api/auth/login', {
            method: 'POST',
            body: auth,
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
            currentView.value = 'totp';
        } else {
            if (!_error.data.data) $toast.error(_error.data.message);
            formErrors.value = _error.data.data?.formErrors;
            await nextTick();
            calculateHeight();
        }

        turnstileRef.value?.reset();
    }

    loggingIn.value = false;
};

const handlePasskeyLogin = async () => {
    passkeyLoggingIn.value = true;

    const { ticket, authenticationOptions } = await $fetch<{
        ticket: string;
        authenticationOptions: PublicKeyCredentialRequestOptionsJSON;
    }>('/api/auth/login/passwordless', {
        method: 'POST',
        body: { verify: false },
        credentials: 'include',
    });

    const authenticationResponse = await startAuthentication({
        optionsJSON: authenticationOptions,
    }).catch(() => null);

    if (authenticationResponse) {
        try {
            const { user, session } = await $fetch<any>('/api/auth/login/passwordless', {
                method: 'POST',
                body: {
                    verify: true,
                    ticket,
                    authenticationResponse,
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
                else {
                    formErrors.value = _error.data.data;
                    await nextTick();
                    calculateHeight();
                }
            } else {
                $toast.error('Failed to verify passkey');
            }
        }
    }

    passkeyLoggingIn.value = false;
};

const calculateHeight = (el?: Element) => {
    height.value =
        (el || document.querySelector('[data-view]')!).clientHeight +
        (runtimeConfig.public.turnstile.siteKey ? 71 : 0);
};

onMounted(async () => {
    const container = document.querySelector('[data-view]');

    if (container) {
        calculateHeight();
        container.querySelector('input')?.focus();
    }
});

definePageMeta({
    middleware: 'guest-only',
});
</script>
