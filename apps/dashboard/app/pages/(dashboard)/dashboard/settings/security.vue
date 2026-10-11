<template>
    <UiCard>
        <form class="contents" @submit.prevent="handlePasswordChange">
            <UiCardHeader>
                <UiCardTitle>Password</UiCardTitle>
                <UiCardDescription>
                    Changing your password signs you out on every other device.
                </UiCardDescription>
            </UiCardHeader>
            <UiCardContent>
                <UiFieldGroup>
                    <UiField :data-invalid="!!passwordError">
                        <UiFieldLabel for="new-password">New password</UiFieldLabel>
                        <UiInput
                            id="new-password"
                            v-model="password.value"
                            type="password"
                            autocomplete="new-password"
                        />
                    </UiField>
                    <UiField :data-invalid="!!passwordError">
                        <UiFieldLabel for="confirm-password">Confirm password</UiFieldLabel>
                        <UiInput
                            id="confirm-password"
                            v-model="password.confirm"
                            type="password"
                            autocomplete="new-password"
                        />
                        <UiFieldError v-if="passwordError" :errors="[passwordError]" />
                    </UiField>
                </UiFieldGroup>
            </UiCardContent>
            <UiCardFooter class="justify-end">
                <UiButton type="submit" :disabled="isChangingPassword || !password.value">
                    <UiSpinner v-if="isChangingPassword" />
                    Change password
                </UiButton>
            </UiCardFooter>
        </form>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle class="flex items-center gap-2">
                Authenticator app
                <UiBadge :variant="currentUser!.totpEnabled ? 'default' : 'secondary'">
                    {{ currentUser!.totpEnabled ? 'Enabled' : 'Disabled' }}
                </UiBadge>
            </UiCardTitle>
            <UiCardDescription>
                Use a code from an authenticator app when signing in and for sensitive actions.
            </UiCardDescription>
            <UiCardAction>
                <UiButton
                    :variant="currentUser!.totpEnabled ? 'outline' : 'default'"
                    :disabled="isUpdatingTotp"
                    @click="currentUser!.totpEnabled ? handleDisableTotp() : handleStartTotp()"
                >
                    <UiSpinner v-if="isUpdatingTotp" />
                    {{ currentUser!.totpEnabled ? 'Disable' : 'Enable' }}
                </UiButton>
            </UiCardAction>
        </UiCardHeader>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>Passkeys</UiCardTitle>
            <UiCardDescription>
                Sign in with your fingerprint, face or a security key.
            </UiCardDescription>
            <UiCardAction>
                <UiButton
                    :disabled="!supportsPasskeys || isRegisteringPasskey"
                    @click="handleRegisterPasskey"
                >
                    <UiSpinner v-if="isRegisteringPasskey" />
                    <IPlus v-else />
                    Add passkey
                </UiButton>
            </UiCardAction>
        </UiCardHeader>
        <UiCardContent class="flex flex-col gap-2">
            <UiAlert v-if="!supportsPasskeys">
                <IInfo />
                <UiAlertTitle>Your browser does not support passkeys.</UiAlertTitle>
            </UiAlert>
            <UiCard v-for="passkey in passkeys" :key="passkey.id" size="sm">
                <UiCardContent class="flex items-center gap-3">
                    <IKeyRound class="size-4 shrink-0 text-muted-foreground" />
                    <div class="flex min-w-0 flex-1 flex-col">
                        <TextBlock variant="small" class="truncate">{{ passkey.name }}</TextBlock>
                        <TextBlock variant="muted" class="text-xs">
                            Added {{ dayjs(passkey.createdAt).fromNow() }}
                        </TextBlock>
                    </div>
                    <UiButton variant="ghost" size="icon-sm" @click="openRename(passkey)">
                        <IPencil />
                    </UiButton>
                    <UiButton variant="ghost" size="icon-sm" @click="handleDeletePasskey(passkey)">
                        <ITrash2 />
                    </UiButton>
                </UiCardContent>
            </UiCard>
            <TextBlock v-if="!passkeys.length && supportsPasskeys" variant="muted">
                No passkeys yet.
            </TextBlock>
        </UiCardContent>
    </UiCard>

    <UiDialog v-model:open="totpDialog.open">
        <UiDialogContent class="sm:max-w-sm">
            <form class="flex flex-col gap-6" @submit.prevent="handleEnableTotp">
                <UiDialogHeader>
                    <UiDialogTitle>Set up authenticator app</UiDialogTitle>
                    <UiDialogDescription>
                        Scan the QR code with your authenticator app, then enter the 6-digit code.
                    </UiDialogDescription>
                </UiDialogHeader>
                <img
                    v-if="totpDialog.qrCode"
                    :src="totpDialog.qrCode"
                    alt="Authenticator QR code"
                    class="mx-auto size-48 rounded-2xl bg-white p-2"
                />
                <UiSkeleton v-else class="mx-auto size-48 rounded-2xl" />
                <UiField :data-invalid="!!totpDialog.error">
                    <OtpInput v-model="totpDialog.code" @complete="handleEnableTotp" />
                    <UiFieldError v-if="totpDialog.error" :errors="[totpDialog.error]" />
                </UiField>
                <UiDialogFooter>
                    <UiButton type="button" variant="outline" @click="totpDialog.open = false">
                        Cancel
                    </UiButton>
                    <UiButton
                        type="submit"
                        :disabled="totpDialog.code.length !== 6 || isUpdatingTotp"
                    >
                        <UiSpinner v-if="isUpdatingTotp" />
                        Enable
                    </UiButton>
                </UiDialogFooter>
            </form>
        </UiDialogContent>
    </UiDialog>

    <UiDialog :open="!!renaming" @update:open="!$event && (renaming = null)">
        <UiDialogContent class="sm:max-w-sm">
            <form class="flex flex-col gap-6" @submit.prevent="handleRenamePasskey">
                <UiDialogHeader>
                    <UiDialogTitle>Rename passkey</UiDialogTitle>
                </UiDialogHeader>
                <UiInput v-model="renameValue" />
                <UiDialogFooter>
                    <UiButton type="button" variant="outline" @click="renaming = null">
                        Cancel
                    </UiButton>
                    <UiButton type="submit" :disabled="!renameValue.trim()">Save</UiButton>
                </UiDialogFooter>
            </form>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

import {
    browserSupportsWebAuthn,
    type PublicKeyCredentialCreationOptionsJSON,
    startRegistration,
} from '@simplewebauthn/browser';

const passkeys = usePasskeys();
const currentUser = useAuthUser();
const { $toast } = useNuxtApp();

const supportsPasskeys = ref(true);
const isChangingPassword = ref(false);
const isUpdatingTotp = ref(false);
const isRegisteringPasskey = ref(false);
const passwordError = ref<string>();
const renaming = ref<PasskeyData | null>(null);
const renameValue = ref('');

const password = reactive({ value: '', confirm: '' });
const totpDialog = reactive({ open: false, qrCode: '', code: '', error: '' });

watch(
    () => totpDialog.open,
    (value) => !value && (totpDialog.qrCode = ''),
);

const handlePasswordChange = async () => {
    passwordError.value = undefined;

    if (password.value !== password.confirm) {
        passwordError.value = 'Passwords do not match';
        return;
    }

    isChangingPassword.value = true;

    try {
        const result = await withVerification((verificationData) =>
            $fetch(`/api/users/${currentUser.value!.id}`, {
                method: 'PATCH',
                body: { password: password.value, verificationData },
            }),
        );

        if (result) {
            Object.assign(password, { value: '', confirm: '' });
            $toast.success('Password changed');
        }
    } catch (error: any) {
        passwordError.value =
            error.data.data?.formErrors?.password?._errors[0] || error.data.message;
    }

    isChangingPassword.value = false;
};

const handleStartTotp = async () => {
    isUpdatingTotp.value = true;

    try {
        const result = await withVerification((verificationData) =>
            $fetch('/api/users/@me/mfa/totp/qrcode', {
                method: 'POST',
                body: { verificationData },
            }),
        );

        if (result) {
            Object.assign(totpDialog, { open: true, qrCode: '', code: '', error: '' });
            setTimeout(() => (totpDialog.qrCode = result.base64), 400);
        }
    } catch (error: any) {
        $toast.error(error.data.message);
    }

    isUpdatingTotp.value = false;
};

const handleEnableTotp = async () => {
    if (totpDialog.code.length !== 6 || isUpdatingTotp.value) return;

    isUpdatingTotp.value = true;
    totpDialog.error = '';

    try {
        await $fetch('/api/users/@me/mfa/totp', {
            method: 'PUT',
            body: { enabled: true, totp: totpDialog.code },
        });

        totpDialog.open = false;
        $toast.success('Authenticator app enabled');
    } catch (error: any) {
        totpDialog.error = error.data.message;
        totpDialog.code = '';
    }

    isUpdatingTotp.value = false;
};

const handleDisableTotp = async () => {
    isUpdatingTotp.value = true;

    try {
        const result = await withVerification((verificationData) =>
            $fetch('/api/users/@me/mfa/totp', {
                method: 'PUT',
                body: { enabled: false, verificationData },
            }),
        );

        if (result) $toast.success('Authenticator app disabled');
    } catch (error: any) {
        $toast.error(error.data.message);
    }

    isUpdatingTotp.value = false;
};

const handleRegisterPasskey = async () => {
    isRegisteringPasskey.value = true;

    try {
        const result = await withVerification(async (verificationData) => {
            const { ticket, registrationOptions } = await $fetch<{
                ticket: string;
                registrationOptions: PublicKeyCredentialCreationOptionsJSON;
            }>('/api/users/@me/mfa/webauthn/credentials', {
                method: 'POST',
                body: { verify: false, verificationData },
            });

            const registrationResponse = await startRegistration({
                optionsJSON: registrationOptions,
            });

            await $fetch('/api/users/@me/mfa/webauthn/credentials', {
                method: 'POST',
                body: { verify: true, ticket, registrationResponse, verificationData },
            });

            return true;
        });

        if (result) $toast.success('Passkey added');
    } catch (error: any) {
        $toast.error(error.data?.message || 'Failed to register passkey');
    }

    isRegisteringPasskey.value = false;
};

const openRename = (passkey: PasskeyData) => {
    renaming.value = passkey;
    renameValue.value = passkey.name;
};

const handleRenamePasskey = async () => {
    const passkey = renaming.value!;
    renaming.value = null;

    try {
        const result = await withVerification((verificationData) =>
            $fetch(`/api/users/@me/mfa/webauthn/credentials/${passkey.id}`, {
                method: 'PATCH',
                body: { name: renameValue.value.trim(), verificationData },
            }),
        );

        if (result) $toast.success('Passkey renamed');
    } catch (error: any) {
        $toast.error(error.data.data?.formErrors?.name?._errors[0] || error.data.message);
    }
};

const handleDeletePasskey = async (passkey: PasskeyData) => {
    await askConfirmation({
        title: `Delete ${passkey.name}?`,
        description: 'You will no longer be able to sign in with this passkey.',
        confirmText: 'Delete',
        destructive: true,
        action: async () => {
            try {
                const result = await withVerification((verificationData) =>
                    $fetch(`/api/users/@me/mfa/webauthn/credentials/${passkey.id}`, {
                        method: 'DELETE',
                        body: { verificationData },
                    }),
                );

                if (result) $toast.success('Passkey deleted');
            } catch (error: any) {
                $toast.error(error.data.message);
            }
        },
    });
};

onMounted(async () => {
    supportsPasskeys.value = browserSupportsWebAuthn();

    const data = await $fetch('/api/users/@me/mfa/webauthn/credentials');
    passkeys.value = data.map((p) => ({ ...p, createdAt: new Date(p.createdAt) }));
});
</script>
