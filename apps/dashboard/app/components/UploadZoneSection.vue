<template>
    <div wfull rounded-xl bg-fs-overlay-2 p6 space-y-6 border="~ fs-overlay-4">
        <h3>
            <template v-if="activeTab === 'Media Upload'">Upload Media Files</template>
            <template v-else>Create Text File</template>
        </h3>

        <DropZone
            v-if="activeTab === 'Media Upload'"
            v-model="uploadingFiles"
            :disabled="disabled"
        />

        <div v-else space-y-4>
            <div flex="~ items-center gap4 <sm:col" wfull>
                <UiInput
                    v-model="textFileData.fileName"
                    label="File Name"
                    type="text"
                    required
                    wfull
                    flex-1
                    wrapper-class="wfull"
                    :disabled="disabled"
                />
                <FileTypePicker v-model="textFileData.fileType">
                    <UiInput
                        v-model="textFileData.fileType.label"
                        label="File Type"
                        type="string"
                        :disabled="disabled"
                        cursor-pointer="!"
                        readonly
                        required
                        wfull
                        wrapper-class="wfull sm:w72"
                    />
                </FileTypePicker>
            </div>

            <UiTextArea
                v-model="textFileData.content"
                :disabled="disabled"
                label="Text"
                required
                wfull
            />
        </div>

        <div flex="~ gap2 items-center">
            <UiButton
                wfull
                gap2
                alignment="center"
                variant="accent"
                icon="solar:upload-minimalistic-linear"
                icon-size="20"
                :disabled="buttonDisabled"
                :loading="disabled"
                @click="emit('upload')"
            >
                <template v-if="activeTab === 'Media Upload'">Upload File(s)</template>
                <template v-else>Create Text</template>
            </UiButton>
        </div>
    </div>
</template>

<script setup lang="ts">
const props = defineProps<{
    activeTab: 'Media Upload' | 'Text Upload';
    disabled?: boolean;
}>();

const emit = defineEmits<{
    upload: [];
}>();

const uploadingFiles = useUploadingFiles();

const textFileData = reactive({
    fileName: '',
    fileType: TEXT_FILE_TYPES[0]!,
    content: '',
});

const buttonDisabled = computed(() => {
    const baseDisabled = props.disabled;
    const emptyFiles = !uploadingFiles.value.length;
    const emptyText = !textFileData.content || !textFileData.fileName;

    return baseDisabled || (emptyFiles && emptyText);
});

defineExpose({
    textFileData,
});
</script>
