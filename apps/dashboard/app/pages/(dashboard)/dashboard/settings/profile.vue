<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>Avatar</UiCardTitle>
            <UiCardDescription>
                A square image of at least 64×64 pixels works best.
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
                    Upload
                </UiButton>
                <UiButton
                    v-if="currentUser!.avatar"
                    variant="outline"
                    :disabled="isUpdatingAvatar"
                    @click="updateAvatar(null)"
                >
                    Remove
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
                <UiCardTitle>Account</UiCardTitle>
                <UiCardDescription>Your public username.</UiCardDescription>
            </UiCardHeader>
            <UiCardContent>
                <UiField :data-invalid="!!formErrors?.username">
                    <UiFieldLabel for="username">Username</UiFieldLabel>
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
                    Save
                </UiButton>
            </UiCardFooter>
        </form>
    </UiCard>
</template>

<script setup lang="ts">
const currentUser = useAuthUser();
const { $toast } = useNuxtApp();

const avatarInput = useTemplateRef('avatarInput');

const username = ref(currentUser.value!.username);
const isSaving = ref(false);
const isUpdatingAvatar = ref(false);
const formErrors = ref();

const updateAvatar = async (avatar: string | null) => {
    isUpdatingAvatar.value = true;

    try {
        await $fetch(`/api/users/${currentUser.value!.id}`, {
            method: 'PATCH',
            body: { avatar },
        });

        $toast.success(avatar ? 'Avatar updated' : 'Avatar removed');
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

        $toast.success('Username updated');
    } catch (error: any) {
        formErrors.value = error.data?.data?.formErrors;
        if (!formErrors.value) $toast.error(getErrorMessage(error));
    }

    isSaving.value = false;
};

watch(
    () => currentUser.value!.username,
    (value) => (username.value = value),
);
</script>
