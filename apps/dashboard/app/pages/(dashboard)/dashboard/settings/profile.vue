<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('settings.profile.avatar') }}</UiCardTitle>
            <UiCardDescription>
                {{ $t('settings.profile.avatarDescription') }}
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent class="flex items-center gap-4">
            <UiAvatar class="size-16">
                <UiAvatarImage v-if="currentUser!.avatar" :src="currentUser!.avatar" />
                <UiAvatarFallback>
                    {{ currentUser!.username.slice(0, 2).toUpperCase() }}
                </UiAvatarFallback>
            </UiAvatar>
            <div class="flex flex-wrap gap-2">
                <UiButton :disabled="isUpdatingAvatar" @click="avatarInput!.click()">
                    <UiSpinner v-if="isUpdatingAvatar" />
                    <IUpload v-else />
                    {{ $t('settings.profile.upload') }}
                </UiButton>
                <UiButton
                    v-if="currentUser!.avatar"
                    variant="outline"
                    :disabled="isUpdatingAvatar"
                    @click="updateAvatar(null)"
                >
                    <ITrash2 />
                    {{ $t('settings.profile.remove') }}
                </UiButton>
            </div>
            <input
                ref="avatarInput"
                type="file"
                accept="image/*"
                class="sr-only"
                @change="handleAvatarChange"
            />
        </UiCardContent>
    </UiCard>

    <UiCard>
        <form class="contents" @submit.prevent="handleSave">
            <UiCardHeader>
                <UiCardTitle>{{ $t('settings.profile.account') }}</UiCardTitle>
                <UiCardDescription>
                    {{ $t('settings.profile.accountDescription') }}
                </UiCardDescription>
            </UiCardHeader>
            <UiCardContent>
                <UiField :data-invalid="!!formErrors?.username">
                    <UiFieldLabel for="username">
                        {{ $t('settings.profile.username') }}
                    </UiFieldLabel>
                    <UiInput id="username" v-model="username" autocomplete="username" />
                    <UiFieldError
                        v-if="formErrors?.username"
                        :errors="formErrors.username._errors"
                    />
                </UiField>
            </UiCardContent>
            <UiCardFooter class="justify-end">
                <UiButton type="submit" :disabled="isSaving || username === currentUser!.username">
                    <UiSpinner v-if="isSaving" />
                    {{ $t('settings.profile.save') }}
                </UiButton>
            </UiCardFooter>
        </form>
    </UiCard>

    <UiCard v-if="publicSettings.emailEnabled">
        <form class="contents" @submit.prevent="handleSaveEmail(email)">
            <UiCardHeader>
                <UiCardTitle>{{ $t('settings.profile.email.title') }}</UiCardTitle>
                <UiCardDescription>
                    {{ $t('settings.profile.email.description') }}
                </UiCardDescription>
            </UiCardHeader>
            <UiCardContent class="flex flex-col gap-4">
                <UiAlert v-if="currentUser!.pendingEmail">
                    <IMailCheck />
                    <UiAlertTitle>{{ $t('settings.profile.email.pendingTitle') }}</UiAlertTitle>
                    <UiAlertDescription>
                        {{
                            currentUser!.email
                                ? $t('settings.profile.email.pendingKeepsCurrent', {
                                      email: currentUser!.pendingEmail,
                                      current: currentUser!.email,
                                  })
                                : $t('settings.profile.email.pendingDescription', {
                                      email: currentUser!.pendingEmail,
                                  })
                        }}
                    </UiAlertDescription>
                </UiAlert>
                <UiField :data-invalid="!!emailErrors?.email">
                    <div class="flex items-center gap-2">
                        <UiFieldLabel for="email">
                            {{ $t('settings.profile.email.label') }}
                        </UiFieldLabel>
                        <UiBadge v-if="currentUser!.pendingEmail" variant="outline">
                            {{ $t('settings.profile.email.notVerified') }}
                        </UiBadge>
                        <UiBadge v-else-if="currentUser!.email" variant="secondary">
                            <IBadgeCheck />
                            {{ $t('settings.profile.email.verified') }}
                        </UiBadge>
                    </div>
                    <UiInput id="email" v-model="email" type="email" autocomplete="email" />
                    <UiFieldError v-if="emailErrors?.email" :errors="emailErrors.email._errors" />
                </UiField>
            </UiCardContent>
            <UiCardFooter class="justify-end gap-2">
                <UiButton
                    v-if="currentUser!.email || currentUser!.pendingEmail"
                    type="button"
                    variant="outline"
                    :disabled="isSavingEmail"
                    @click="handleRemoveEmail"
                >
                    {{ $t('settings.profile.email.remove') }}
                </UiButton>
                <UiButton
                    v-if="currentUser!.pendingEmail"
                    type="button"
                    variant="outline"
                    :disabled="isSavingEmail"
                    @click="handleSaveEmail(currentUser!.pendingEmail)"
                >
                    {{ $t('settings.profile.email.resend') }}
                </UiButton>
                <UiButton type="submit" :disabled="isSavingEmail || !isEmailChanged">
                    <UiSpinner v-if="isSavingEmail" />
                    {{ $t('settings.profile.save') }}
                </UiButton>
            </UiCardFooter>
        </form>
    </UiCard>
</template>

<script setup lang="ts">
const { t } = useI18n();
const currentUser = useAuthUser();
const publicSettings = usePublicSettings();
const { $toast } = useNuxtApp();

const avatarInput = useTemplateRef('avatarInput');

const username = ref(currentUser.value!.username);
const isSaving = ref(false);
const isUpdatingAvatar = ref(false);
const formErrors = ref();

const currentEmail = computed(
    () => currentUser.value!.pendingEmail ?? currentUser.value!.email ?? '',
);
const email = ref(currentEmail.value);
const isSavingEmail = ref(false);
const emailErrors = ref();

const isEmailChanged = computed(
    () => !!email.value.trim() && email.value.trim().toLowerCase() !== currentEmail.value,
);

const updateAvatar = async (avatar: string | null) => {
    isUpdatingAvatar.value = true;

    try {
        await $fetch(`/api/users/${currentUser.value!.id}`, {
            method: 'PATCH',
            body: { avatar },
        });

        $toast.success(
            avatar ? t('settings.profile.avatarUpdated') : t('settings.profile.avatarRemoved'),
        );
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    isUpdatingAvatar.value = false;
};

const handleAvatarChange = (event: Event) => {
    const input = event.target as HTMLInputElement;
    const reader = new FileReader();

    reader.onload = () => updateAvatar((reader.result as string).split(',')[1]!);
    reader.readAsDataURL(input.files![0]!);

    input.value = '';
};

const handleSave = async () => {
    isSaving.value = true;
    formErrors.value = null;

    try {
        await $fetch(`/api/users/${currentUser.value!.id}`, {
            method: 'PATCH',
            body: { username: username.value },
        });

        $toast.success(t('settings.profile.usernameUpdated'));
    } catch (error: any) {
        formErrors.value = error.data?.data?.formErrors;
        if (!formErrors.value) $toast.error(getErrorMessage(error));
    }

    isSaving.value = false;
};

const handleSaveEmail = async (value: string) => {
    isSavingEmail.value = true;
    emailErrors.value = null;

    try {
        const result = await withVerification((verificationData) =>
            $fetch('/api/users/@me/email', {
                method: 'PUT',
                body: { email: value, verificationData },
            }),
        );

        if (result) {
            Object.assign(currentUser.value!, result);
            $toast.success(
                result.pendingEmail
                    ? t('settings.profile.email.sent', { email: result.pendingEmail })
                    : t('settings.profile.email.pendingCancelled'),
            );
        }
    } catch (error: any) {
        emailErrors.value = error.data?.data?.formErrors;
        if (!emailErrors.value) $toast.error(getErrorMessage(error));
    }

    isSavingEmail.value = false;
};

const handleRemoveEmail = () =>
    askConfirmation({
        title: t('settings.profile.email.removeTitle'),
        description: t('settings.profile.email.removeDescription'),
        confirmText: t('settings.profile.email.remove'),
        destructive: true,
        action: async () => {
            try {
                const result = await withVerification((verificationData) =>
                    $fetch('/api/users/@me/email', {
                        method: 'DELETE',
                        body: { verificationData },
                    }),
                );

                if (result) {
                    Object.assign(currentUser.value!, result);
                    $toast.success(t('settings.profile.email.removed'));
                }
            } catch (error) {
                $toast.error(getErrorMessage(error));
                return false;
            }
        },
    });

watch(
    () => currentUser.value!.username,
    (value) => (username.value = value),
);

watch(currentEmail, (value) => (email.value = value));
</script>
