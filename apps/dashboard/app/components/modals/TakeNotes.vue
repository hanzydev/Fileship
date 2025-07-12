<template>
    <UiModal
        v-model="isOpen"
        @closed="
            note.title = '';
            note.content = '';
            formErrors = {};
        "
    >
        <form p8 space-y-4 @submit.prevent="handleSubmit">
            <h2>Take Notes</h2>

            <UiInput
                v-model="note.title"
                label="Title"
                type="text"
                :error="formErrors?.title?._errors?.[0]"
                :disabled
                required
                wfull
            />
            <UiTextArea
                v-model="note.content"
                label="Content"
                :error="formErrors?.content?._errors?.[0]"
                :disabled
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
const isOpen = defineModel<boolean>({ required: true });

const { $toast } = useNuxtApp();

const formErrors = ref();
const disabled = ref(false);

const note = reactive({
    title: '',
    content: '',
});

const handleSubmit = async () => {
    disabled.value = true;
    formErrors.value = {};

    try {
        await $fetch('/api/notes', {
            method: 'POST',
            body: note,
        });

        isOpen.value = false;

        $toast.success('Note taken successfully');
    } catch (error: any) {
        formErrors.value = error.data.data?.formErrors;
    }

    disabled.value = false;
};
</script>
