<template>
    <ModalsVerifyTotp
        v-if="currentUser!.totpEnabled"
        v-model="verifyModalOpen"
        :error="verificationError"
        :disabled="updating"
        @got="handleEdit"
        @cancel="isOpen = true"
        @outer-click="isOpen = true"
    />
    <ModalsVerifyUserPassword
        v-else
        v-model="verifyModalOpen"
        :error="verificationError"
        :disabled="updating"
        @got="handleEdit"
        @cancel="isOpen = true"
        @outer-click="isOpen = true"
    />

    <UiModal v-model="isOpen">
        <form p8 space-y-4 @submit.prevent="handleEdit()">
            <h2>Edit User</h2>

            <UiInput
                v-model="editData.cloned.value!.username"
                label="Username"
                type="text"
                :error="formErrors?.username?._errors?.[0]"
                required
                wfull
                :disabled="updating"
            />
            <UiInput
                v-model="editData.cloned.value!.password!"
                wfull
                label="Password"
                type="password"
                :error="formErrors?.password?._errors?.[0]"
                :disabled="updating"
            />

            <UiDropdown placement="top" pb0.5="!">
                <UiInput
                    :model-value="editData.cloned.value!.permissions.join(', ')"
                    :required="!editData.cloned.value!.superAdmin"
                    readonly
                    wfull
                    label="Permissions"
                    type="text"
                    :error="formErrors?.permissions?._errors?.[0]"
                    :disabled="editData.cloned.value!.superAdmin || updating"
                    :cursor-pointer="
                        editData.cloned.value!.superAdmin ? '' : '!'
                    "
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
                            :variant="
                                editData.cloned.value!.permissions.includes(
                                    permission,
                                )
                                    ? 'accent'
                                    : 'primary'
                            "
                            :icon="
                                editData.cloned.value!.permissions.includes(
                                    permission,
                                )
                                    ? 'heroicons-solid:check'
                                    : PermissionIcon[permission]
                            "
                            wfull
                            gap2.5
                            icon-size="20"
                            :disabled="
                                editData.cloned.value!.permissions.includes(
                                    UserPermission.Admin,
                                ) && permission !== UserPermission.Admin
                            "
                            @click="
                                editData.cloned.value!.permissions.includes(
                                    permission,
                                )
                                    ? editData.cloned.value!.permissions.splice(
                                          editData.cloned.value!.permissions.indexOf(
                                              permission,
                                          ),
                                          1,
                                      )
                                    : editData.cloned.value!.permissions.push(
                                          permission,
                                      )
                            "
                        >
                            {{ titleCase(permission) }}
                        </UiButton>
                    </div>
                </template>
            </UiDropdown>
            <div grid="~ sm:cols-2 gap4">
                <UiInput
                    v-model="editData.cloned.value!.limits.backupLimit"
                    required
                    wfull
                    label="Backup Limit"
                    caption="Set to -1 for unlimited backups"
                    type="number"
                    :min="-1"
                    :disabled="updating"
                />
                <UiInput
                    v-model="editData.cloned.value!.limits.usableSpace"
                    required
                    wfull
                    label="Usable Space, in MB"
                    caption="Set to -1 for unlimited space"
                    type="number"
                    :min="-1"
                    :disabled="updating"
                />
            </div>

            <div v-if="currentUser!.superAdmin" flex="~ gap2 items-center">
                <UiSwitch
                    v-model="editData.cloned.value!.superAdmin"
                    :disabled="updating"
                />
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
                    :loading="updating"
                    :disabled="updating"
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

import { PermissionIcon } from '~~/utils/user';

const { data } = defineProps<{
    data: UserData;
}>();

const isOpen = defineModel<boolean>({ required: true });

const currentUser = useAuthUser();

const formErrors = ref();
const updating = ref(false);

const verifyModalOpen = ref(false);
const verificationError = ref<string>();

const editData = useCloned({
    ...data,
    password: '',
    permissions: data.superAdmin ? [] : data.permissions,
});

const handleEdit = async (verificationData?: string) => {
    updating.value = true;
    formErrors.value = {};

    try {
        await $fetch(`/api/users/${data!.id}`, {
            method: 'PATCH',
            body: {
                username: editData.cloned.value!.username,
                password: editData.cloned.value!.password || undefined,
                permissions: editData.cloned.value!.permissions,
                limits: editData.cloned.value!.limits,
                superAdmin: editData.cloned.value!.superAdmin,
                verificationData,
            },
        });

        isOpen.value = false;
        verifyModalOpen.value = false;

        toast.success('User updated successfully');
    } catch (error: any) {
        if (!error.data.data) {
            if (verifyModalOpen.value) {
                verificationError.value = error.data.message;
            } else if (error.data.message === 'Verification is required') {
                verifyModalOpen.value = true;
                isOpen.value = false;
            } else if (!verifyModalOpen.value) {
                toast.error(error.data.message);
            }
        }

        formErrors.value = error.data.data;
    }

    updating.value = false;
};

watch(
    () => editData.cloned.value.permissions,
    (value) => {
        if (value.includes(UserPermission.Admin) && value.length > 1) {
            editData.cloned.value.permissions = [UserPermission.Admin];
        }
    },
    { deep: true },
);

watch(
    () => editData.cloned.value.superAdmin,
    (value) => {
        if (value) editData.cloned.value.permissions = [];
    },
);
</script>
