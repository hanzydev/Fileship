<template>
    <UiDialog :open="!!files.length" @update:open="!$event && (files = [])">
        <UiDialogContent class="sm:max-w-md">
            <UiDialogHeader>
                <UiDialogTitle>
                    {{ $t('files.moveTitle', files.length) }}
                </UiDialogTitle>
                <UiDialogDescription>{{ $t('files.moveDescription') }}</UiDialogDescription>
            </UiDialogHeader>
            <FolderPicker v-model="folderId" />
            <UiDialogFooter>
                <UiButton variant="outline" @click="files = []">{{ $t('common.cancel') }}</UiButton>
                <UiButton :disabled="isMoving" @click="handleMove">
                    <UiSpinner v-if="isMoving" />
                    {{ $t('files.move') }}
                </UiButton>
            </UiDialogFooter>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
import type { FileData } from '~/types/files';

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
    if (!value.length) return;
    folderId.value = value.every((file) => file.folderId === value[0]!.folderId)
        ? value[0]!.folderId
        : null;
});
</script>
