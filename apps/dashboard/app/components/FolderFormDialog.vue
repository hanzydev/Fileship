<template>
    <UiDialog v-model:open="open">
        <UiDialogContent class="sm:max-w-md">
            <form class="flex flex-col gap-6" @submit.prevent="handleSubmit">
                <UiDialogHeader>
                    <UiDialogTitle>
                        {{ folder ? $t('folders.form.editTitle') : $t('folders.form.newTitle') }}
                    </UiDialogTitle>
                    <UiDialogDescription>
                        {{
                            folder
                                ? $t('folders.form.editDescription')
                                : $t('folders.form.newDescription')
                        }}
                    </UiDialogDescription>
                </UiDialogHeader>

                <UiFieldGroup>
                    <UiField :data-invalid="!!formErrors?.name">
                        <UiFieldLabel for="folder-name">{{ $t('folders.form.name') }}</UiFieldLabel>
                        <UiInput
                            id="folder-name"
                            v-model="form.name"
                            :placeholder="$t('folders.form.namePlaceholder')"
                        />
                        <UiFieldError v-if="formErrors?.name" :errors="formErrors.name._errors" />
                    </UiField>
                    <UiField
                        orientation="horizontal"
                        class="has-[>[data-slot=field-content]]:items-center"
                    >
                        <UiFieldContent>
                            <UiFieldLabel for="folder-public">
                                {{ $t('folders.form.public') }}
                            </UiFieldLabel>
                            <UiFieldDescription>
                                {{ $t('folders.form.publicDescription') }}
                            </UiFieldDescription>
                        </UiFieldContent>
                        <UiSwitch id="folder-public" v-model="form.public" />
                    </UiField>
                </UiFieldGroup>

                <UiDialogFooter>
                    <UiButton type="button" variant="outline" @click="open = false">
                        {{ $t('common.cancel') }}
                    </UiButton>
                    <UiButton type="submit" :disabled="isSaving || !form.name.trim()">
                        <UiSpinner v-if="isSaving" />
                        {{ folder ? $t('folders.form.save') : $t('folders.form.create') }}
                    </UiButton>
                </UiDialogFooter>
            </form>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
import type { FolderData } from '~/types/folders';

const { folder } = defineProps<{
    folder?: FolderData | null;
}>();

const open = defineModel<boolean>('open', { required: true });

const { $toast } = useNuxtApp();
const { t } = useI18n();

const isSaving = ref(false);
const formErrors = ref();
const form = reactive({ name: '', public: false });

const handleSubmit = async () => {
    isSaving.value = true;
    formErrors.value = null;

    try {
        if (folder) {
            await $fetch(`/api/folders/${folder.id}`, { method: 'PATCH', body: form });
            $toast.success(t('folders.form.updated'));
        } else {
            const created = await $fetch('/api/folders', { method: 'POST', body: form });
            $toast.success(t('folders.form.created'));
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
