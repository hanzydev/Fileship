<template>
    <ModalsVerifyMFA
        v-model="verifyModalOpen"
        :error="verificationError"
        :disabled
        :methods="verificationMethods"
        @got="handleSubmit"
        @cancel="isOpen = true"
        @outer-click="isOpen = true"
    />

    <UiModal
        v-model="isOpen"
        @closed="
            () => {
                if (!verifyModalOpen) {
                    user.username = '';
                    user.password = '';
                    user.permissions = [];
                    user.superAdmin = false;
                    user.limits = { backupLimit: -1, usableSpace: -1 };
                    formErrors = {};
                }
            }
        "
    >
        <form p8 space-y-4 @submit.prevent="handleSubmit()">
            <h2>Create User</h2>

            <UiInput
                v-model="user.username"
                label="Username"
                type="text"
                :error="formErrors?.username?._errors?.[0]"
                required
                wfull
                :disabled
            />
            <UiInput
                v-model="user.password"
                required
                wfull
                label="Password"
                type="password"
                :error="formErrors?.password?._errors?.[0]"
                :disabled
            />
            <UiDropdown placement="top" pb0.5="!">
                <UiInput
                    :model-value="user.permissions.join(', ')"
                    :required="!user.superAdmin"
                    readonly
                    wfull
                    label="Permissions"
                    type="text"
                    :error="formErrors?.permissions?._errors?.[0]"
                    :disabled="user.superAdmin || disabled"
                    :cursor-pointer="user.superAdmin ? '' : '!'"
                />

                <template #content>
                    <div
                        relative
                        top-6
                        w52
                        rounded-lg
                        bg-fs-overlay-2
                        p1.5
                        ring="1 fs-accent"
                        space-y-1
                    >
                        <UiButton
                            v-for="permission in Object.values(UserPermission)"
                            :key="permission"
                            :variant="user.permissions.includes(permission) ? 'accent' : 'onOverlay'"
                            :icon="
                                user.permissions.includes(permission)
                                    ? 'heroicons-solid:check'
                                    : PermissionIcon[permission]
                            "
                            wfull
                            gap2.5
                            icon-size="20"
                            :disabled="
                                user.permissions.includes(UserPermission.Admin) &&
                                permission !== UserPermission.Admin
                            "
                            @click="
                                user.permissions.includes(permission)
                                    ? user.permissions.splice(
                                          user.permissions.indexOf(permission),
                                          1,
                                      )
                                    : user.permissions.push(permission)
                            "
                        >
                            {{ titleCase(permission) }}
                        </UiButton>
                    </div>
                </template>
            </UiDropdown>
            <div grid="~ sm:cols-2 gap4">
                <UiInput
                    v-model="user.limits.backupLimit"
                    required
                    wfull
                    label="Backup Limit"
                    caption="Set to -1 for unlimited backups"
                    type="number"
                    :min="-1"
                    :disabled
                />
                <UiInput
                    v-model="user.limits.usableSpace"
                    required
                    wfull
                    label="Usable Space, in MB"
                    caption="Set to -1 for unlimited space"
                    type="number"
                    :min="-1"
                    :disabled
                />
            </div>

            <div v-if="currentUser!.superAdmin" flex="~ gap2 items-center">
                <UiSwitch v-model="user.superAdmin" :disabled />
                <span font-medium="!">Super admin</span>
            </div>

            <div grid="~ cols-2 gap4">
                <UiButton
                    alignment="center"
                    icon="heroicons-solid:x"
                    icon-size="24"
                    wfull
                    gap2
                    @click="isOpen = false"
                >
                    Cancel
                </UiButton>
                <UiButton
                    wfull
                    gap2
                    alignment="center"
                    variant="accent"
                    type="submit"
                    icon="heroicons:pencil-16-solid"
                    icon-size="20"
                    :loading="disabled"
                    :disabled
                >
                    Save
                </UiButton>
            </div>
        </form>
    </UiModal>
</template>

<script setup lang="ts">
import { titleCase } from 'scule';
import { toast } from 'vue-sonner';

import { UserPermission } from '@prisma/client';

const isOpen = defineModel<boolean>({ required: true });

const currentUser = useAuthUser();

const formErrors = ref();
const disabled = ref(false);

const verifyModalOpen = ref(false);
const verificationError = ref<string>();
const verificationMethods = ref([]);

const user = reactive({
    username: '',
    password: '',
    permissions: [] as UserPermission[],
    superAdmin: false,
    limits: { backupLimit: -1, usableSpace: -1 },
});

const handleSubmit = async (verificationData?: any) => {
    disabled.value = true;
    formErrors.value = {};
    verificationError.value = undefined;

    try {
        await $fetch('/api/users', {
            method: 'POST',
            body: {
                ...user,
                verificationData,
            },
        });

        isOpen.value = false;
        verifyModalOpen.value = false;

        toast.success('User created successfully');
    } catch (error: any) {
        if (!error.data.data?.formErrors) {
            if (verifyModalOpen.value) {
                verificationError.value = error.data.message;
            } else if (error.data.message === 'Verification is required') {
                verifyModalOpen.value = true;
                verificationMethods.value = error.data.data.mfa.methods;
                isOpen.value = false;
            } else if (!verifyModalOpen.value) {
                toast.error(error.data.message);
            }
        }

        formErrors.value = error.data.data?.formErrors;
    }

    disabled.value = false;
};

watch(
    () => user.permissions,
    (value) => {
        if (value.includes(UserPermission.Admin) && value.length > 1) {
            user.permissions = [UserPermission.Admin];
        }
    },
    { deep: true },
);

watch(
    () => user.superAdmin,
    (value) => {
        if (value) user.permissions = [];
    },
);
</script>
