<template>
    <UiSheet v-model:open="open">
        <UiSheetContent class="data-[side=right]:w-full data-[side=right]:sm:max-w-md">
            <form class="flex flex-1 flex-col overflow-hidden" @submit.prevent="handleSubmit">
                <UiSheetHeader>
                    <UiSheetTitle>{{ user ? `Edit ${user.username}` : 'New user' }}</UiSheetTitle>
                    <UiSheetDescription>
                        {{
                            user
                                ? 'Update account details, permissions and limits.'
                                : 'Create an account for someone else.'
                        }}
                    </UiSheetDescription>
                </UiSheetHeader>

                <div class="flex flex-1 flex-col gap-6 overflow-y-auto px-6 pb-6">
                    <UiFieldGroup>
                        <UiField :data-invalid="!!formErrors?.username">
                            <UiFieldLabel for="user-username">Username</UiFieldLabel>
                            <UiInput
                                id="user-username"
                                v-model="form.username"
                                autocomplete="off"
                            />
                            <UiFieldError
                                v-if="formErrors?.username"
                                :errors="formErrors.username._errors"
                            />
                        </UiField>
                        <UiField :data-invalid="!!formErrors?.password">
                            <UiFieldLabel for="user-password">Password</UiFieldLabel>
                            <UiInput
                                id="user-password"
                                v-model="form.password"
                                type="password"
                                autocomplete="new-password"
                                :placeholder="user ? 'Leave empty to keep the current one' : ''"
                            />
                            <UiFieldError
                                v-if="formErrors?.password"
                                :errors="formErrors.password._errors"
                            />
                        </UiField>
                    </UiFieldGroup>

                    <UiFieldSet v-if="!form.superAdmin">
                        <UiFieldLegend>Permissions</UiFieldLegend>
                        <UiFieldGroup class="gap-3">
                            <UiField
                                v-for="permission in Object.values(UserPermission)"
                                :key="permission"
                                orientation="horizontal"
                            >
                                <UiCheckbox
                                    :id="`permission-${permission}`"
                                    :model-value="form.permissions.includes(permission)"
                                    :disabled="
                                        permission !== UserPermission.Admin &&
                                        form.permissions.includes(UserPermission.Admin)
                                    "
                                    @update:model-value="togglePermission(permission)"
                                />
                                <UiFieldLabel :for="`permission-${permission}`">
                                    {{ titleCase(permission) }}
                                </UiFieldLabel>
                            </UiField>
                        </UiFieldGroup>
                        <UiFieldError
                            v-if="formErrors?.permissions"
                            :errors="formErrors.permissions._errors"
                        />
                    </UiFieldSet>

                    <UiFieldSet>
                        <UiFieldLegend>Limits</UiFieldLegend>
                        <UiFieldDescription>Use -1 for unlimited.</UiFieldDescription>
                        <div class="grid grid-cols-2 gap-4">
                            <UiField>
                                <UiFieldLabel>Backups</UiFieldLabel>
                                <UiNumberField v-model="form.limits.backupLimit" :min="-1">
                                    <UiNumberFieldContent>
                                        <UiNumberFieldDecrement />
                                        <UiNumberFieldInput />
                                        <UiNumberFieldIncrement />
                                    </UiNumberFieldContent>
                                </UiNumberField>
                            </UiField>
                            <UiField>
                                <UiFieldLabel>Storage (MB)</UiFieldLabel>
                                <UiNumberField v-model="form.limits.usableSpace" :min="-1">
                                    <UiNumberFieldContent>
                                        <UiNumberFieldDecrement />
                                        <UiNumberFieldInput />
                                        <UiNumberFieldIncrement />
                                    </UiNumberFieldContent>
                                </UiNumberField>
                            </UiField>
                        </div>
                    </UiFieldSet>

                    <UiField v-if="currentUser!.superAdmin" orientation="horizontal">
                        <UiFieldContent>
                            <UiFieldLabel for="user-super-admin">Super admin</UiFieldLabel>
                            <UiFieldDescription>
                                Full access, including managing other admins.
                            </UiFieldDescription>
                        </UiFieldContent>
                        <UiSwitch id="user-super-admin" v-model="form.superAdmin" />
                    </UiField>
                </div>

                <UiSheetFooter>
                    <UiButton type="submit" :disabled="isSaving">
                        <UiSpinner v-if="isSaving" />
                        {{ user ? 'Save changes' : 'Create user' }}
                    </UiButton>
                    <UiSheetClose as-child>
                        <UiButton type="button" variant="outline">Cancel</UiButton>
                    </UiSheetClose>
                </UiSheetFooter>
            </form>
        </UiSheetContent>
    </UiSheet>
</template>

<script setup lang="ts">
import { titleCase } from 'scule';

import { UserPermission } from '#shared/prisma/enums';

const { user } = defineProps<{
    user?: UserData | null;
}>();

const open = defineModel<boolean>('open', { required: true });

const currentUser = useAuthUser();
const { $toast } = useNuxtApp();

const isSaving = ref(false);
const formErrors = ref();

const form = reactive({
    username: '',
    password: '',
    permissions: [] as UserPermission[],
    superAdmin: false,
    limits: { backupLimit: -1, usableSpace: -1 },
});

const togglePermission = (permission: UserPermission) => {
    if (form.permissions.includes(permission)) {
        form.permissions = form.permissions.filter((p) => p !== permission);
    } else {
        form.permissions =
            permission === UserPermission.Admin ? [permission] : [...form.permissions, permission];
    }
};

const handleSubmit = async () => {
    isSaving.value = true;
    formErrors.value = null;

    const body = {
        ...form,
        password: user && !form.password ? undefined : form.password,
        permissions: form.superAdmin ? [] : form.permissions,
    };

    try {
        const result = await withVerification<unknown>((verificationData) =>
            user
                ? $fetch(`/api/users/${user.id}`, {
                      method: 'PATCH',
                      body: { ...body, verificationData },
                  })
                : $fetch('/api/users', { method: 'POST', body: { ...body, verificationData } }),
        );

        if (result) {
            open.value = false;
            $toast.success(user ? 'User updated' : 'User created');
        }
    } catch (error: any) {
        formErrors.value = error.data?.data?.formErrors;
        if (!formErrors.value) $toast.error(getErrorMessage(error));
    }

    isSaving.value = false;
};

watch(open, (value) => {
    if (!value) return;

    formErrors.value = null;
    Object.assign(form, {
        username: user?.username || '',
        password: '',
        permissions: [...(user?.permissions || [])],
        superAdmin: user?.superAdmin || false,
        limits: { ...defaultUserLimits, ...(user?.limits as any) },
    });
});
</script>
