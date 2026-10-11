<template>
    <UiDialog :open="!!files.length" @update:open="!$event && (files = [])">
        <UiDialogContent class="sm:max-w-md">
            <UiDialogHeader>
                <UiDialogTitle>
                    Move {{ files.length > 1 ? `${files.length} files` : 'file' }}
                </UiDialogTitle>
                <UiDialogDescription>Choose a folder or create a new one.</UiDialogDescription>
            </UiDialogHeader>
            <FolderPicker v-model="folderId" />
            <UiDialogFooter>
                <UiButton variant="outline" @click="files = []">Cancel</UiButton>
                <UiButton :disabled="isMoving" @click="handleMove">
                    <UiSpinner v-if="isMoving" />
                    Move
                </UiButton>
            </UiDialogFooter>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
const files = defineModel<Pick<FileData, 'id' | 'folderId'>[]>({ required: true });

const emit = defineEmits<{
    moved: [];
}>();

const { move } = useFileActions();

const folderId = ref<string | null>(null);
const isMoving = ref(false);

const handleMove = async () => {
    isMoving.value = true;

    if (await move(files.value, folderId.value)) {
        files.value = [];
        emit('moved');
    }

    isMoving.value = false;
};

watch(files, (value) => {
    if (value.length) folderId.value = value.length === 1 ? value[0]!.folderId : null;
});
</script>
