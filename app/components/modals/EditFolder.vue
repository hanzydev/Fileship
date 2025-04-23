<template>
    <UiModal v-model="isOpen">
        <form p8 space-y-4 @submit.prevent="handleEdit">
            <h2>Edit Folder</h2>

            <UiInput
                v-model="editData.cloned.value!.name"
                label="Name"
                type="text"
                :error="formErrors?.name?._errors?.[0]"
                required
                wfull
                :disabled="updating"
            />
            <div flex="~ gap2 items-center">
                <UiSwitch v-model="editData.cloned.value!.public" :disabled="updating" />
                <span text-neutral200 font-medium="!">Public</span>
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
import { toast } from 'vue-sonner';

const { data } = defineProps<{
    data: FolderData;
}>();

const isOpen = defineModel<boolean>({ required: true });

const formErrors = ref();
const updating = ref(false);

const editData = useCloned(data);

const handleEdit = async () => {
    updating.value = true;
    formErrors.value = {};

    try {
        await $fetch(`/api/folders/${data!.id}`, {
            method: 'PATCH',
            body: {
                name: editData.cloned.value!.name,
                public: editData.cloned.value!.public,
            },
        });

        isOpen.value = false;

        toast.success('Folder updated successfully');
    } catch (error: any) {
        if (!error.data.data?.formErrors) toast.error(error.data.message);
        formErrors.value = error.data.data?.formErrors;
    }

    updating.value = false;
};

watch(
    () => data,
    (value) => (editData.cloned.value = JSON.parse(JSON.stringify(value))),
);
</script>
