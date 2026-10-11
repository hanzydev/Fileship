<template>
    <UiDialog v-model:open="open">
        <UiDialogContent class="sm:max-w-md">
            <form class="flex flex-col gap-6" @submit.prevent="handleSubmit">
                <UiDialogHeader>
                    <UiDialogTitle>{{ folder ? 'Edit folder' : 'New folder' }}</UiDialogTitle>
                    <UiDialogDescription>
                        {{
                            folder
                                ? 'Rename the folder or change who can see it.'
                                : 'Group files together and share them with a single link.'
                        }}
                    </UiDialogDescription>
                </UiDialogHeader>

                <UiFieldGroup>
                    <UiField :data-invalid="!!formErrors?.name">
                        <UiFieldLabel for="folder-name">Name</UiFieldLabel>
                        <UiInput
                            id="folder-name"
                            v-model="form.name"
                            placeholder="Holiday photos"
                        />
                        <UiFieldError v-if="formErrors?.name" :errors="formErrors.name._errors" />
                    </UiField>
                    <UiField orientation="horizontal">
                        <UiFieldContent>
                            <UiFieldLabel for="folder-public">Public</UiFieldLabel>
                            <UiFieldDescription>
                                Anyone with the link can view the files in this folder.
                            </UiFieldDescription>
                        </UiFieldContent>
                        <UiSwitch id="folder-public" v-model="form.public" />
                    </UiField>
                </UiFieldGroup>

                <UiDialogFooter>
                    <UiButton type="button" variant="outline" @click="open = false">
                        Cancel
                    </UiButton>
                    <UiButton type="submit" :disabled="isSaving || !form.name.trim()">
                        <UiSpinner v-if="isSaving" />
                        {{ folder ? 'Save' : 'Create' }}
                    </UiButton>
                </UiDialogFooter>
            </form>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
const { folder } = defineProps<{
    folder?: FolderData | null;
}>();

const open = defineModel<boolean>('open', { required: true });

const { $toast } = useNuxtApp();

const isSaving = ref(false);
const formErrors = ref();
const form = reactive({ name: '', public: false });

const handleSubmit = async () => {
    isSaving.value = true;
    formErrors.value = null;

    try {
        if (folder) {
            await $fetch(`/api/folders/${folder.id}`, { method: 'PATCH', body: form });
            $toast.success('Folder updated');
        } else {
            const created = await $fetch('/api/folders', { method: 'POST', body: form });
            $toast.success('Folder created');
            navigateTo(`/dashboard/folders/${created.id}`);
        }

        open.value = false;
    } catch (error: any) {
        formErrors.value = error.data?.data?.formErrors;
        if (!formErrors.value) $toast.error(getErrorMessage(error));
    }

    isSaving.value = false;
};

watch(open, (value) => {
    if (!value) return;

    formErrors.value = null;
    Object.assign(form, { name: folder?.name || '', public: folder?.public || false });
});
</script>
