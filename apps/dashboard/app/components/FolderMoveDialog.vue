<template>
    <UiDialog :open="!!folder" @update:open="!$event && (folder = null)">
        <UiDialogContent class="sm:max-w-md">
            <UiDialogHeader>
                <UiDialogTitle>
                    {{ $t('folders.moveDialog.title', { name: shown?.name }) }}
                </UiDialogTitle>
                <UiDialogDescription>
                    {{ $t('folders.moveDialog.description') }}
                </UiDialogDescription>
            </UiDialogHeader>
            <FolderPicker
                v-model="parentId"
                :exclude="shown?.id"
                :none-label="$t('folders.moveDialog.topLevel')"
            />
            <UiDialogFooter>
                <UiButton variant="outline" @click="folder = null">
                    {{ $t('common.cancel') }}
                </UiButton>
                <UiButton
                    :disabled="isMoving || parentId === (shown?.parentId ?? null)"
                    @click="handleMove"
                >
                    <UiSpinner v-if="isMoving" />
                    {{ $t('folders.moveDialog.move') }}
                </UiButton>
            </UiDialogFooter>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
import type { FolderData } from '~/types/folders';

const folder = defineModel<FolderData | null>({ required: true });

const { move } = useFolderActions();

const shown = ref(folder.value);
const parentId = ref<string | null>(null);
const isMoving = ref(false);

const handleMove = async () => {
    isMoving.value = true;
    if (await move(shown.value!, parentId.value)) folder.value = null;
    isMoving.value = false;
};

watch(folder, (value) => {
    if (!value) return;

    shown.value = value;
    parentId.value = value.parentId;
});
</script>
