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
                    enter-active-class="motion-safe:animate-in motion-safe:fade-in motion-safe:slide-in-left-52"
                    leave-active-class="motion-safe:animate-out motion-safe:fade-out motion-safe:slide-out-left-52"
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
                    </form>
                </Transition>
                <Transition
                    enter-active-class="motion-safe:animate-in motion-safe:fade-in motion-safe:slide-in-right-52"
                    leave-active-class="motion-safe:animate-out motion-safe:fade-out motion-safe:slide-out-right-52"
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
                                    Enter the code from your authenticator app
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

                        <UiButton
                            alignment="center"
                            wfull
                            gap2
                            variant="accent"
                            type="submit"
                            icon="heroicons-solid:arrow-left"
                            icon-size="20"
                            :loading="disabled"
                            :disabled
                            @click="section = 'login'"
                        >
                            Back
                        </UiButton>
                    </div>
                </Transition>
            </div>
        </UiCentered>
    </div>
</template>

<script setup lang="ts">
import { toast } from 'vue-sonner';

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

const section = ref<'login' | 'totp'>('login');

const route = useRoute();
const currentUser = useAuthUser();

const handleSubmit = async (totp?: string) => {
    disabled.value = true;
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

        await navigateTo((route.query.redirectTo as string) || '/dashboard');

        toast.success('Logged in successfully');
    } catch (_error: any) {
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

definePageMeta({
    middleware: 'guest-only',
});
</script>
