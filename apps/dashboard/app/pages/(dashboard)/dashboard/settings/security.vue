<template>
    <UiCard>
        <form class="contents" @submit.prevent="handlePasswordChange">
            <UiCardHeader>
                <UiCardTitle>{{ $t('security.password') }}</UiCardTitle>
                <UiCardDescription>
                    {{ $t('security.passwordDescription') }}
                </UiCardDescription>
            </UiCardHeader>
            <UiCardContent>
                <UiFieldGroup>
                    <UiField :data-invalid="!!passwordError">
                        <UiFieldLabel for="new-password">
                            {{ $t('security.newPassword') }}
                        </UiFieldLabel>
                        <PasswordInput
                            id="new-password"
                            v-model="password.value"
                            autocomplete="new-password"
                        />
                    </UiField>
                    <UiField :data-invalid="!!passwordError">
                        <UiFieldLabel for="confirm-password">
                            {{ $t('security.confirmPassword') }}
                        </UiFieldLabel>
                        <PasswordInput
                            id="confirm-password"
                            v-model="password.confirm"
                            autocomplete="new-password"
                        />
                        <UiFieldError v-if="passwordError" :errors="[passwordError]" />
                    </UiField>
                </UiFieldGroup>
            </UiCardContent>
            <UiCardFooter class="justify-end">
                <UiButton type="submit" :disabled="isChangingPassword || !password.value">
                    <UiSpinner v-if="isChangingPassword" />
                    {{ $t('security.changePassword') }}
                </UiButton>
            </UiCardFooter>
        </form>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle class="flex items-center gap-2">
                {{ $t('security.authenticatorApp') }}
                <UiBadge :variant="currentUser!.totpEnabled ? 'default' : 'secondary'">
                    {{
                        currentUser!.totpEnabled ? $t('security.enabled') : $t('security.disabled')
                    }}
                </UiBadge>
            </UiCardTitle>
            <UiCardDescription>
                {{ $t('security.authenticatorAppDescription') }}
            </UiCardDescription>
            <UiCardAction class="self-center">
                <UiButton
                    :variant="currentUser!.totpEnabled ? 'outline' : 'default'"
                    :disabled="isUpdatingTotp"
                    @click="currentUser!.totpEnabled ? handleDisableTotp() : handleStartTotp()"
                >
                    <UiSpinner v-if="isUpdatingTotp" />
                    <IShieldOff v-else-if="currentUser!.totpEnabled" />
                    <IShieldCheck v-else />
                    {{ currentUser!.totpEnabled ? $t('security.disable') : $t('security.enable') }}
                </UiButton>
            </UiCardAction>
        </UiCardHeader>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('security.passkeys') }}</UiCardTitle>
            <UiCardDescription>
                {{ $t('security.passkeysDescription') }}
            </UiCardDescription>
            <UiCardAction>
                <UiButton
                    :disabled="!supportsPasskeys || isRegisteringPasskey"
                    @click="handleRegisterPasskey"
                >
                    <UiSpinner v-if="isRegisteringPasskey" />
                    <IPlus v-else />
                    {{ $t('security.addPasskey') }}
                </UiButton>
            </UiCardAction>
        </UiCardHeader>
        <UiCardContent class="flex flex-col gap-2">
            <UiAlert v-if="!supportsPasskeys">
                <IInfo />
                <UiAlertTitle>{{ $t('security.passkeysUnsupported') }}</UiAlertTitle>
            </UiAlert>
            <template v-if="isLoadingPasskeys">
                <ListCardSkeleton v-for="i in 2" :key="i" media="size-4" />
            </template>
            <EmptyState
                v-else-if="!passkeys.length && supportsPasskeys"
                :icon="IKeyRound"
                :title="$t('security.noPasskeys')"
                :description="$t('security.noPasskeysDescription')"
            />
            <UiCard v-for="passkey in passkeys" v-else :key="passkey.id" size="sm">
                <UiCardContent class="flex min-h-8 items-center gap-3">
                    <IKeyRound class="size-4 shrink-0 text-muted-foreground" />
                    <div class="flex min-w-0 flex-1 flex-col">
                        <TextBlock variant="small" class="truncate">{{ passkey.name }}</TextBlock>
                        <TextBlock variant="muted" class="text-xs">
                            {{
                                $t('security.passkeyAdded', {
                                    time: date(passkey.createdAt).fromNow(),
                                })
                            }}
                        </TextBlock>
                    </div>
                    <UiDropdownMenu>
                        <UiDropdownMenuTrigger as-child>
                            <UiButton variant="ghost" size="icon-sm">
                                <IEllipsis />
                            </UiButton>
                        </UiDropdownMenuTrigger>
                        <UiDropdownMenuContent align="end" class="w-56">
                            <UiDropdownMenuItem @select="openRename(passkey)">
                                <IPencil />
                                {{ $t('security.rename') }}
                            </UiDropdownMenuItem>
                            <UiDropdownMenuSeparator />
                            <UiDropdownMenuItem
                                variant="destructive"
                                @select="handleDeletePasskey(passkey)"
                            >
                                <ITrash2 />
                                {{ $t('security.delete') }}
                            </UiDropdownMenuItem>
                        </UiDropdownMenuContent>
                    </UiDropdownMenu>
                </UiCardContent>
            </UiCard>
        </UiCardContent>
    </UiCard>

    <UiDialog v-model:open="totpDialog.open">
        <UiDialogContent class="sm:max-w-sm">
            <form class="flex flex-col gap-6" @submit.prevent="handleEnableTotp">
                <UiDialogHeader>
                    <UiDialogTitle>{{ $t('security.setupTitle') }}</UiDialogTitle>
                    <UiDialogDescription>
                        {{ $t('security.setupDescription') }}
                    </UiDialogDescription>
                </UiDialogHeader>
                <img
                    v-if="totpDialog.qrCode"
                    :src="totpDialog.qrCode"
                    :alt="$t('security.qrCode')"
                    class="mx-auto size-48 rounded-2xl bg-white p-2"
                />
                <UiSkeleton v-else class="mx-auto size-48" />
                <UiField :data-invalid="!!totpDialog.error">
                    <OtpInput v-model="totpDialog.code" @complete="handleEnableTotp" />
                    <UiFieldError v-if="totpDialog.error" :errors="[totpDialog.error]" />
                </UiField>
                <UiDialogFooter>
                    <UiButton type="button" variant="outline" @click="totpDialog.open = false">
                        {{ $t('common.cancel') }}
                    </UiButton>
                    <UiButton
                        type="submit"
                        :disabled="totpDialog.code.length !== 6 || isUpdatingTotp"
                    >
                        <UiSpinner v-if="isUpdatingTotp" />
                        {{ $t('security.enable') }}
                    </UiButton>
                </UiDialogFooter>
            </form>
        </UiDialogContent>
    </UiDialog>

    <UiDialog :open="!!renaming" @update:open="!$event && (renaming = null)">
        <UiDialogContent class="sm:max-w-sm">
            <form class="flex flex-col gap-6" @submit.prevent="handleRenamePasskey">
                <UiDialogHeader>
                    <UiDialogTitle>{{ $t('security.renameTitle') }}</UiDialogTitle>
                    <UiDialogDescription>
                        {{ $t('security.renameDescription') }}
                    </UiDialogDescription>
                </UiDialogHeader>
                <UiField>
                    <UiFieldLabel for="passkey-name">{{ $t('security.name') }}</UiFieldLabel>
                    <UiInput id="passkey-name" v-model="renameValue" maxlength="32" />
                </UiField>
                <UiDialogFooter>
                    <UiButton type="button" variant="outline" @click="renaming = null">
                        {{ $t('common.cancel') }}
                    </UiButton>
                    <UiButton type="submit" :disabled="!renameValue.trim() || isRenaming">
                        <UiSpinner v-if="isRenaming" />
                        {{ $t('security.save') }}
                    </UiButton>
                </UiDialogFooter>
            </form>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
import {
    browserSupportsWebAuthn,
    bufferToBase64URLString,
    type PublicKeyCredentialCreationOptionsJSON,
    sendSignal,
    startRegistration,
} from '@simplewebauthn/browser';

import { IKeyRound } from '#components';

const passkeys = usePasskeys();
const isLoadingPasskeys = ref(!passkeys.value.length);
const isRenaming = ref(false);
const currentUser = useAuthUser();
const { $toast } = useNuxtApp();
const { t } = useI18n();
const date = useDate();

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
        passwordError.value = t('security.passwordMismatch');
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
            $toast.success(t('security.passwordChanged'));
        }
    } catch (error: any) {
        passwordError.value =
            error.data?.data?.formErrors?.password?._errors[0] || getErrorMessage(error);
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
        $toast.error(getErrorMessage(error));
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
        $toast.success(t('security.totpEnabled'));
    } catch (error: any) {
        totpDialog.error = getErrorMessage(error);
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

        if (result) $toast.success(t('security.totpDisabled'));
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
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

            const { verified } = await $fetch<{ verified: boolean }>(
                '/api/users/@me/mfa/webauthn/credentials',
                {
                    method: 'POST',
                    body: { verify: true, ticket, registrationResponse, verificationData },
                },
            );

            return verified;
        });

        if (result) $toast.success(t('security.passkeyAddedToast'));
        else if (result === false) $toast.error(t('security.passkeyNotVerified'));
    } catch (error: any) {
        $toast.error(error.data?.message || t('security.passkeyAddFailed'));
    }

    isRegisteringPasskey.value = false;
};

const openRename = (passkey: PasskeyData) => {
    renaming.value = passkey;
    renameValue.value = passkey.name;
};

const handleRenamePasskey = async () => {
    const passkey = renaming.value!;
    isRenaming.value = true;

    try {
        const result = await withVerification((verificationData) =>
            $fetch(`/api/users/@me/mfa/webauthn/credentials/${passkey.id}`, {
                method: 'PATCH',
                body: { name: renameValue.value.trim(), verificationData },
            }),
        );

        if (result) {
            renaming.value = null;
            $toast.success(t('security.passkeyRenamed'));
        }
    } catch (error: any) {
        $toast.error(error.data?.data?.formErrors?.name?._errors[0] || getErrorMessage(error));
    }

    isRenaming.value = false;
};

const handleDeletePasskey = async (passkey: PasskeyData) => {
    await askConfirmation({
        title: t('security.deleteTitle', { name: passkey.name }),
        description: t('security.deleteDescription'),
        confirmText: t('security.delete'),
        destructive: true,
        action: async () => {
            try {
                const result = await withVerification((verificationData) =>
                    $fetch(`/api/users/@me/mfa/webauthn/credentials/${passkey.id}`, {
                        method: 'DELETE',
                        body: { verificationData },
                    }),
                );

                if (result) $toast.success(t('security.passkeyDeleted'));
            } catch (error: any) {
                $toast.error(getErrorMessage(error));
            }
        },
    });
};

onMounted(async () => {
    supportsPasskeys.value = browserSupportsWebAuthn();

    const data = await $fetch('/api/users/@me/mfa/webauthn/credentials').catch(() => null);
    passkeys.value = (data || []).map((p) => ({ ...p, createdAt: new Date(p.createdAt) }));
    isLoadingPasskeys.value = false;

    if (data) {
        await sendSignal({
            signalName: 'allAcceptedCredentials',
            rpID: location.hostname,
            userID: bufferToBase64URLString(new TextEncoder().encode(currentUser.value!.id).buffer),
            allAcceptedCredentialIDs: data.map((p) => p.id),
        }).catch(() => null);
    }
});
</script>
