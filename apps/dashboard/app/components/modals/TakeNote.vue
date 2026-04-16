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
            <h2>Take a Note</h2>

            <UiInput
                v-model="note.title"
                label="Title"
                type="text"
                :error="formErrors?.title?._errors?.[0]"
                :disabled
                required
                wfull
                rounded-xl="!"
            />
            <div space-y-1>
                <UiLabel :error="formErrors?.content?._errors?.[0]" required>Content</UiLabel>
                <div relative>
                    <UiButton
                        alignment="center"
                        variant="glass"
                        :icon="previewMode ? 'solar:pen-2-bold' : 'solar:eye-bold'"
                        icon-size="16"
                        aria-label="previewMode ? 'Preview' : 'Edit'"
                        absolute
                        right-3.5
                        top-3.5
                        z-10
                        gap-1.5
                        px2.5!
                        py0.75!
                        text-sm!
                        :disabled
                        @click="previewMode = !previewMode"
                    >
                        {{ previewMode ? 'Edit' : 'Preview' }}
                    </UiButton>
                    <UiTextArea
                        v-if="!previewMode"
                        v-model="note.content"
                        :disabled
                        required
                        wfull
                        rounded-xl="!"
                    />
                    <div
                        v-else
                        border="~ fs-overlay-4"
                        max-h-500px
                        min-h-250px
                        overflow-auto
                        rounded-xl
                        bg-fs-overlay-3
                    >
                        <MarkdownRenderer
                            variant="secondary"
                            :content="note.content"
                            px3.5!
                            py2.5!
                        />
                    </div>
                </div>
            </div>

            <div flex="~ gap2 items-center">
                <UiSwitch v-model="note.public" :disabled />
                <span text-fs-muted-1 font-medium="!">Public</span>
            </div>

            <div grid="~ cols-2 gap4">
                <UiButton
                    alignment="center"
                    icon="lucide:x"
                    icon-size="24"
                    wfull
                    gap2
                    rounded-xl="!"
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
                    icon="solar:pen-2-bold"
                    icon-size="20"
                    :loading="disabled"
                    :disabled
                    rounded-xl="!"
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
const previewMode = ref(false);

const note = reactive({
    title: '',
    content: '',
    public: false,
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
