<template>
    <UiDialog :open="!!verification" @update:open="!$event && verification?.cancel()">
        <UiDialogContent v-if="verification" class="sm:max-w-sm">
            <form class="flex flex-col gap-6" @submit.prevent="handleSubmit">
                <UiDialogHeader>
                    <UiDialogTitle>Confirm it's you</UiDialogTitle>
                    <UiDialogDescription>
                        This action needs an extra verification step.
                    </UiDialogDescription>
                </UiDialogHeader>

                <UiTabs v-if="methods.length > 1" v-model="method">
                    <UiTabsList class="w-full">
                        <UiTabsTrigger v-for="m in methods" :key="m.type" :value="m.type">
                            {{ labels[m.type] }}
                        </UiTabsTrigger>
                    </UiTabsList>
                </UiTabs>

                <TextBlock v-if="method === 'passkey'" variant="muted">
                    Confirm with your fingerprint, face or security key.
                </TextBlock>

                <UiField v-else-if="method === 'totp'">
                    <UiFieldLabel>Authenticator code</UiFieldLabel>
                    <OtpInput v-model="totp" @complete="handleSubmit" />
                </UiField>

                <UiField v-else>
                    <UiFieldLabel for="verify-password">Password</UiFieldLabel>
                    <UiInput
                        id="verify-password"
                        v-model="password"
                        type="password"
                        autocomplete="current-password"
                    />
                </UiField>

                <UiAlert v-if="verification.error" variant="destructive">
                    <ICircleAlert />
                    <UiAlertTitle>{{ verification.error }}</UiAlertTitle>
                </UiAlert>

                <UiDialogFooter>
                    <UiButton type="button" variant="outline" @click="verification.cancel()">
                        Cancel
                    </UiButton>
                    <UiButton
                        v-if="method === 'passkey'"
                        type="button"
                        :disabled="verification.isSubmitting"
                        @click="handlePasskey"
                    >
                        <UiSpinner v-if="verification.isSubmitting" />
                        <IFingerprint v-else />
                        Use a passkey
                    </UiButton>
                    <UiButton
                        v-else
                        type="submit"
                        :disabled="verification.isSubmitting || !canSubmit"
                    >
                        <UiSpinner v-if="verification.isSubmitting" />
                        Verify
                    </UiButton>
                </UiDialogFooter>
            </form>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
import { startAuthentication } from '@simplewebauthn/browser';

const verification = useVerification();
const { $toast } = useNuxtApp();

const labels = { passkey: 'Passkey', totp: 'Code', password: 'Password' };

const method = ref<VerificationMethod['type']>('password');
const totp = ref('');
const password = ref('');

const methods = computed(() => verification.value?.methods || []);

const canSubmit = computed(() =>
    method.value === 'totp' ? totp.value.length === 6 : !!password.value,
);

const handleSubmit = () => {
    if (!canSubmit.value || verification.value!.isSubmitting) return;

    verification.value!.submit({
        type: method.value,
        data: method.value === 'totp' ? totp.value : password.value,
    });
};

const handlePasskey = async () => {
    const { ticket, authenticationOptions } = methods.value.find(
        (m) => m.type === 'passkey',
    ) as any;

    try {
        const authenticationResponse = await startAuthentication({
            optionsJSON: authenticationOptions,
        });

        verification.value!.submit({ type: 'passkey', data: { ticket, authenticationResponse } });
    } catch {
        $toast.error('Failed to verify passkey');
    }
};

watch(
    () => verification.value?.error,
    (value) => value && (totp.value = ''),
);

watch(methods, (value) => {
    if (!value.length) return;

    totp.value = '';
    password.value = '';
    method.value = value.some((m) => m.type === 'passkey')
        ? 'passkey'
        : value.some((m) => m.type === 'totp')
          ? 'totp'
          : 'password';
});
</script>
