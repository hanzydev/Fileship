<template>
    <div>
        <ModalsVerifyMFA
            v-model="verifyModalOpen"
            :methods="verificationMethods"
            :error="verificationError"
            :disabled="updating"
            @got="handleEdit"
        />

        <UiModal v-model="isOpen">
            <form p8 space-y-4 @submit.prevent="handleEdit()">
                <h2>Edit Passkey</h2>

                <UiInput
                    v-model="editData.cloned.value!.name"
                    label="Name"
                    type="text"
                    :error="formErrors?.name?._errors?.[0]"
                    :disabled="updating"
                    required
                    wfull
                />

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
    </div>
</template>

<script setup lang="ts">
const { data } = defineProps<{ data: PasskeyData }>();

const isOpen = defineModel<boolean>({ required: true });

const { $toast } = useNuxtApp();

const formErrors = ref();
const updating = ref(false);

const verifyModalOpen = ref(false);
const verificationError = ref<string>();
const verificationMethods = ref([]);

const editData = useCloned(data);

const handleEdit = async (verificationData?: any) => {
    updating.value = true;

    try {
        await $fetch(`/api/users/@me/mfa/webauthn/credentials/${data.id}`, {
            method: 'PATCH',
            body: { name: editData.cloned.value.name, verificationData },
        });

        verifyModalOpen.value = false;
        isOpen.value = false;

        $toast.success('Passkey updated successfully');
    } catch (error: any) {
        if (verifyModalOpen.value) {
            verificationError.value = error.data.message;
        } else if (error.data?.data?.mfa?.methods) {
            isOpen.value = false;
            verifyModalOpen.value = true;
            verificationMethods.value = error.data.data.mfa.methods;
        } else if (error?.data?.data?.formErrors) {
            formErrors.value = error.data.data.formErrors;
        }
    }

    updating.value = false;
};

watch(
    () => data,
    (value) => (editData.cloned.value = JSON.parse(JSON.stringify(value))),
);
</script>
