<template>
    <UiModal v-model="isOpen">
        <form p8 space-y-4 @submit.prevent="handleEdit">
            <h2>Edit File</h2>

            <UiInput
                v-model="editData.cloned.value!.fileName"
                label="File Name"
                type="text"
                :error="formErrors?.fileName?._errors?.[0]"
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
            <UiInput
                v-model="editData.cloned.value!.maxViews"
                wfull
                label="Max Views"
                caption="Set to 0 for unlimited views"
                type="number"
                :min="0"
                :error="formErrors?.maxViews?._errors?.[0]"
                :disabled="updating"
            />
            <ExpirationPicker
                ref="expirationPickerRef"
                v-model="editData.cloned.value!.expiration"
            >
                <UiInput
                    v-model="editData.cloned.value!.expiration.label"
                    label="Expiration"
                    type="string"
                    :error="formErrors?.expiresAt?._errors?.[0]"
                    :disabled="updating"
                    readonly
                    wfull
                    cursor-pointer="!"
                />
            </ExpirationPicker>

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
import { toast } from 'vue-sonner';

import { ExpirationPicker } from '#components';

const { data } = defineProps<{
    data: FileData;
}>();

const isOpen = defineModel<boolean>({ required: true });

const formErrors = ref();
const updating = ref(false);

const expirationPickerRef = ref<InstanceType<typeof ExpirationPicker>>();
const expiration = computed(() =>
    data.expiresAt && expirationPickerRef.value
        ? expirationPickerRef.value.predictExpiration(data.expiresAt)
        : {
              label: 'Never',
              value: null as number | null,
          },
);

const editData = useCloned({
    ...data,
    fileName: getBasename(data.fileName),
    expiration: expiration.value!,
});

const handleEdit = async () => {
    updating.value = true;
    formErrors.value = {};

    try {
        await $fetch(`/api/files/${data!.id}`, {
            method: 'PATCH',
            body: {
                fileName: `${editData.cloned.value!.fileName}${getExtname(data.fileName)}`,
                password: editData.cloned.value!.password,
                maxViews: editData.cloned.value!.maxViews,
                expiration: editData.cloned.value!.expiration.value,
                folderId: editData.cloned.value!.folderId,
            },
        });

        isOpen.value = false;

        toast.success('File updated successfully');
    } catch (error: any) {
        if (!error.data.data) toast.error(error.data.message);
        formErrors.value = error.data.data;
    }

    updating.value = false;
};
</script>
