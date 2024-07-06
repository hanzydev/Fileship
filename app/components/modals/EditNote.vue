<template>
    <UiModal v-model="isOpen">
        <form p8 space-y-4 @submit.prevent="handleEdit">
            <h2>Edit Note</h2>

            <UiInput
                v-model="editData.cloned.value!.title"
                label="Title"
                type="text"
                :error="formErrors?.title?._errors?.[0]"
                :disabled="updating"
                required
                wfull
            />
            <UiTextArea
                v-model="editData.cloned.value!.content"
                label="Content"
                :error="formErrors?.content?._errors?.[0]"
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
</template>

<script setup lang="ts">
import { toast } from 'vue-sonner';

const { data } = defineProps<{
    data: NoteData;
}>();

const isOpen = defineModel<boolean>({ required: true });

const formErrors = ref();
const updating = ref(false);

const editData = useCloned(data);

const handleEdit = async () => {
    updating.value = true;
    formErrors.value = {};

    try {
        await $fetch(`/api/notes/${data!.id}`, {
            method: 'PATCH',
            body: {
                title: editData.cloned.value!.title,
                content: editData.cloned.value!.content,
            },
        });

        isOpen.value = false;

        toast.success('Note updated successfully');
    } catch (error: any) {
        if (!error.data.data) toast.error(error.data.message);
        formErrors.value = error.data.data;
    }

    updating.value = false;
};
</script>
