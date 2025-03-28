<template>
    <Transition
        enter-active-class="motion-safe:(animate-in fade-in zoom-in-95 slide-in-bottom-2)"
        leave-active-class="motion-safe:(animate-out fade-out zoom-out-95 slide-out-bottom-2)"
    >
        <div v-if="isOpen" fixed bottom-6 z30 wfull rounded-xl md="right-6 w40rem" lt-md:px4>
            <UiDropdown placement="top" wfull md:w40rem>
                <UiButton
                    :icon="
                        isErrored
                            ? 'heroicons-solid:exclamation-circle'
                            : 'heroicons-solid:information-circle'
                    "
                    py3="!"
                    :class="isErrored ? '!ring-red-500' : 'ring-fs-accent'"
                    :icon-class="isErrored && '!text-red-500'"
                    icon-size="20"
                    wfull
                    gap2
                    rounded-xl
                    ring-1
                >
                    <template v-if="isErrored">Some file(s) have errors</template>
                    <template v-else>
                        {{ unfinishedFiles.length }}
                        file(s)
                        {{ uploading ? 'uploading' : 'queued' }}
                        <UiProgress v-if="uploading" :value="progress" mla />
                    </template>
                </UiButton>

                <template #content>
                    <div
                        absolute
                        bottom-0
                        mb2
                        max-h40rem
                        wfull
                        overflow-y-auto
                        rounded-xl
                        bg-fs-overlay-2
                        p4
                        ring-1
                        md:w-40rem
                        space-y-4
                        :class="isErrored ? 'ring-red-500' : 'ring-fs-accent'"
                        v-bind="containerProps"
                    >
                        <div space-y-2>
                            <div flex="~ items-center gap2">
                                <Icon name="heroicons-solid:document-duplicate" size="24" />
                                <h3>{{ uploadingFiles.length }} file(s)</h3>
                            </div>
                            <UiDivider />
                        </div>

                        <div flex="~ items-center" wfull rounded-md bg-fs-overlay-3 px3 py2>
                            <h6 w="2/4">File</h6>
                            <h6 w="1/4">Size</h6>
                            <h6 w="2/4">Status</h6>
                            <h6 w="1/4">Progress</h6>
                        </div>

                        <div v-bind="wrapperProps">
                            <UiDropdown v-for="{ data: file } in list" :key="file.name" as-ctx-menu>
                                <div flex="~ items-center" mx3 h10>
                                    <span
                                        line-clamp-1
                                        w="2/4"
                                        break-all
                                        text-sm="!"
                                        font-medium="!"
                                    >
                                        {{ file.name }}
                                    </span>
                                    <span w="1/4" text-sm="!" font-medium="!">
                                        {{ filesize(file.size) }}
                                    </span>
                                    <div
                                        flex="~ items-center gap2"
                                        w="2/4"
                                        text-sm="!"
                                        font-medium="!"
                                    >
                                        <template v-if="file.status?.error">
                                            <Icon
                                                name="heroicons-solid:exclamation-circle"
                                                size="16"
                                                flex-shrink-0
                                                text-red-500
                                            />
                                            {{ file.status?.error }}
                                        </template>
                                        <template v-else-if="file.status?.progress === 100">
                                            <Icon
                                                name="heroicons-solid:check-circle"
                                                size="16"
                                                flex-shrink-0
                                                text-fs-accent
                                            />
                                            Finished
                                        </template>
                                        <template v-else-if="!file.status?.started">
                                            <Icon
                                                name="heroicons-solid:clock"
                                                size="16"
                                                flex-shrink-0
                                                text-yellow-500
                                            />
                                            Queued
                                        </template>

                                        <template v-else-if="file.status?.progress">
                                            <Icon
                                                name="heroicons-solid:upload"
                                                size="16"
                                                flex-shrink-0
                                            />
                                            Uploading
                                        </template>
                                        <template v-else-if="file.status?.started">
                                            <Icon
                                                name="heroicons:rocket-launch-20-solid"
                                                size="16"
                                                flex-shrink-0
                                                text-green-500
                                            />
                                            Starting
                                        </template>
                                    </div>
                                    <UiProgress
                                        :value="file.status?.progress || 0"
                                        w="1/4"
                                        text-class="w1/3"
                                    />
                                </div>
                                <template #content>
                                    <div w40 rounded-xl bg-fs-overlay-2 p1.5 ring="1 fs-overlay-4">
                                        <UiButton
                                            variant="onOverlay"
                                            wfull
                                            gap2
                                            text-red-500
                                            py1="!"
                                            icon="heroicons-solid:x"
                                            icon-size="20"
                                            :disabled="file.status?.started"
                                            @click="
                                                uploadingFiles = uploadingFiles.filter(
                                                    (f) => f !== file,
                                                )
                                            "
                                        >
                                            Remove
                                        </UiButton>
                                    </div>
                                </template>
                            </UiDropdown>
                        </div>
                    </div>
                </template>
            </UiDropdown>
        </div>
    </Transition>
</template>

<script setup lang="ts">
import { filesize } from 'filesize';

const uploadingFiles = useUploadingFiles();
const uploading = useIsUploading();

const isOpen = ref(false);

const isErrored = computed(
    () => !uploading.value && uploadingFiles.value.some((file) => file.status?.error),
);

const progress = computed(() =>
    Math.round(
        uploadingFiles.value.reduce((acc, file) => acc + (file.status?.progress || 0), 0) /
            uploadingFiles.value.length,
    ),
);

const unfinishedFiles = computed(() =>
    uploadingFiles.value.filter((file) => file.status?.progress !== 100),
);

const { list, containerProps, wrapperProps } = useVirtualList(uploadingFiles, {
    itemHeight: 40,
});

watch(uploadingFiles, async (files) => {
    if (files.length && !isOpen.value) {
        isOpen.value = true;
    } else if (!files.length && isOpen.value) {
        isOpen.value = false;
    }
});
</script>
