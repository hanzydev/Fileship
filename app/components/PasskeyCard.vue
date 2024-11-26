<template>
    <div>
        <ModalsVerifyMFA
            v-model="verifyModalOpen"
            :methods="verificationMethods"
            :error="verificationError"
            :disabled="deleting"
            @got="handleDelete"
        />

        <ModalsEditPasskey v-model="editModalOpen" :data="data" />

        <div rounded bg-fs-overlay-3 p="2 l3" flex="~ items-center justify-between gap2">
            <span line-clamp-1 font-medium>
                {{ data.name }}
            </span>
            <UiDropdown placement="right" pl1.5="!">
                <UiButton
                    variant="tertiary"
                    icon="heroicons-solid:cog"
                    icon-size="20"
                    p0="!"
                    h8
                    w8
                    flex-shrink-0
                    alignment="center"
                    rounded-sm
                />
                <template #content>
                    <div w36 rounded-lg bg-fs-overlay-2 p1.5 space-y-1 ring="1 fs-accent">
                        <UiButton
                            icon="heroicons:pencil-16-solid"
                            icon-size="20"
                            wfull
                            gap2
                            @click="(editModalOpen = true)"
                        >
                            Edit
                        </UiButton>
                        <UiButton
                            icon="heroicons-solid:trash"
                            icon-size="20"
                            wfull
                            gap2
                            text-red-500
                            :disabled="deleting"
                            @click="handleDelete()"
                        >
                            Delete
                        </UiButton>
                    </div>
                </template>
            </UiDropdown>
        </div>
    </div>
</template>

<script setup lang="ts">
import { toast } from 'vue-sonner';

const { data } = defineProps<{ data: PasskeyData }>();

const deleting = ref(false);

const verifyModalOpen = ref(false);
const verificationError = ref<string>();
const verificationMethods = ref([]);

const editModalOpen = ref(false);

const handleDelete = async (verificationData?: any) => {
    deleting.value = true;

    try {
        await $fetch(`/api/users/@me/mfa/webauthn/credentials/${data.id}`, {
            method: 'DELETE',
            body: { verificationData },
        });

        verifyModalOpen.value = false;

        toast.success('Passkey deleted successfully');
    } catch (error: any) {
        if (verifyModalOpen.value) {
            verificationError.value = error.data.message;
        } else {
            verifyModalOpen.value = true;
            verificationMethods.value = error.data.data.mfa.methods;
        }
    }

    deleting.value = false;
};
</script>
