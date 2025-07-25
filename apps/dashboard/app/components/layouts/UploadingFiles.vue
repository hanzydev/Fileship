<template>
    <Transition
        enter-active-class="motion-safe:(animate-in fade-in zoom-in-95 slide-in-bottom-2)"
        leave-active-class="motion-safe:(animate-out fade-out zoom-out-95 slide-out-bottom-2)"
    >
        <div v-if="isOpen" fixed bottom-6 z30 wfull rounded-xl md="right-6 w45rem" lt-md:px4>
            <UiDropdown placement="top" wfull md:w45rem>
                <UiButton
                    :icon="
                        isErrored
                            ? 'heroicons-solid:exclamation-circle'
                            : 'heroicons-solid:information-circle'
                    "
                    py3="!"
                    :class="isErrored ? '!ring-red-500' : 'ring-fs-overlay-4'"
                    :icon-class="isErrored && '!text-red-500'"
                    icon-size="20"
                    wfull
                    gap2
                    rounded-xl="!"
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
                        max-h45rem
                        wfull
                        overflow-y-auto
                        rounded-xl
                        bg-fs-overlay-2
                        p4
                        ring-1
                        md:w-45rem
                        space-y-4
                        :class="isErrored ? 'ring-red-500' : 'ring-fs-overlay-4'"
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
                            <h6 w="3.2/4">Progress</h6>
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
                                        <template
                                            v-else-if="file.status?.progress?.percent === 100"
                                        >
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

                                        <template v-else-if="file.status?.progress?.percent">
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
                                    <div flex="~ items-center gap-1" w="3.2/4">
                                        <template
                                            v-if="
                                                file.status?.progress?.speed && !file.status?.error
                                            "
                                        >
                                            <span mra text-sm>
                                                {{ filesize(file.status.progress.speed) }}/s
                                            </span>
                                            <span>|</span>
                                            <span mxa text-sm>
                                                ETA:
                                                {{
                                                    dayjs
                                                        .duration(
                                                            file.status.progress.eta,
                                                            'seconds',
                                                        )
                                                        .format('HH:mm:ss')
                                                }}
                                            </span>
                                            <span>|</span>
                                        </template>
                                        <UiProgress
                                            :value="file.status?.progress?.percent || 0"
                                            :class="
                                                file.status?.progress?.speed && !file.status?.error
                                                    ? 'mla'
                                                    : 'mra'
                                            "
                                        />
                                    </div>
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
                                            :disabled="!file.status?.error && file.status?.started"
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
import dayjs from 'dayjs';
import { filesize } from 'filesize';

const uploadingFiles = useUploadingFiles();
const uploading = useIsUploading();

const isOpen = ref(false);

const isErrored = computed(
    () => !uploading.value && uploadingFiles.value.some((file) => file.status?.error),
);

const progress = computed(() =>
    clamp(
        Math.round(
            uploadingFiles.value.reduce(
                (acc, file) => acc + (file.status?.progress?.percent || 0),
                0,
            ) / uploadingFiles.value.length,
        ),
        0,
        100,
    ),
);

const unfinishedFiles = computed(() =>
    uploadingFiles.value.filter((file) => file.status?.progress?.percent !== 100),
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
