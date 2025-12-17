<template>
    <ModalsViewFolder
        v-if="createdFolder"
        v-model="selectFilesModalOpen"
        :data="createdFolder!"
        new-folder
    />

    <UiModal
        v-model="createModalOpen"
        @closed="
            folder.name = '';
            folder.public = false;
            folder.files = [];
            formErrors = {};
        "
    >
        <form p8 space-y-4 @submit.prevent="handleSubmit">
            <h2>Create Folder</h2>

            <UiInput
                v-model="folder.name"
                label="Name"
                type="text"
                :error="formErrors?.name?._errors?.[0]"
                required
                wfull
                rounded-xl="!"
                :disabled
            />
            <div flex="~ gap2 items-center">
                <UiSwitch v-model="folder.public" :disabled />
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
                    @click="createModalOpen = false"
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
                    rounded-xl="!"
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
const createModalOpen = defineModel<boolean>({ required: true });

const { $toast } = useNuxtApp();

const selectFilesModalOpen = ref(false);
const formErrors = ref();
const disabled = ref(false);

const folder = reactive({
    name: '',
    public: false,
    files: [],
});

const createdFolder = ref<FolderData>();

const handleSubmit = async () => {
    disabled.value = true;
    formErrors.value = {};

    try {
        const folderData = await $fetch('/api/folders', {
            method: 'POST',
            body: folder,
        });

        createdFolder.value = {
            ...folderData,
            createdAt: new Date(folderData.createdAt),
        };

        await nextTick();

        createModalOpen.value = false;
        selectFilesModalOpen.value = true;

        $toast.success('Folder created successfully, now select files to add');
    } catch (error: any) {
        formErrors.value = error.data.data?.formErrors;
    }

    disabled.value = false;
};
</script>
