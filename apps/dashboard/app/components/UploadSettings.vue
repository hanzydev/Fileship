<template>
    <div space-y-6>
        <div>
            <h3 mb-4>Upload Settings</h3>
            <div space-y-4>
                <div v-if="showFileNameType" space-y-2>
                    <UiLabel :for="`filename-${id}`">File Name Type</UiLabel>
                    <UiTabs
                        :id="`filename-${id}`"
                        :model-value="settings.fileNameType"
                        variant="secondary"
                        :items="[
                            {
                                label: 'Random',
                                icon: 'solar:box-bold',
                            },
                            {
                                label: 'UUID',
                                icon: 'solar:key-bold',
                            },
                            {
                                label: 'Original',
                                icon: 'solar:document-bold',
                            },
                        ]"
                        rounded-xl="!"
                        button-class="rounded-lg!"
                        width-full
                        @update:model-value="(val) => emit('update:fileNameType', val as never)"
                    />
                </div>

                <div space-y-1>
                    <UiInput
                        :model-value="settings.password ?? ''"
                        wfull
                        label="Password"
                        type="password"
                        :disabled="disabled"
                        @update:model-value="
                            (val) => emit('update:password', (val as string) || null)
                        "
                    />
                </div>

                <div space-y-1>
                    <UiInput
                        :model-value="settings.maxViews || 0"
                        wfull
                        label="Max Views"
                        caption="Set to 0 for unlimited views."
                        type="number"
                        :min="0"
                        :disabled="disabled"
                        @update:model-value="(val) => emit('update:maxViews', +val)"
                    />
                </div>
            </div>
        </div>

        <div space-y-3>
            <h4 text-sm>Distribution</h4>
            <div space-y-4>
                <ExpirationPicker
                    :model-value="settings.expiration"
                    @update:model-value="(val) => emit('update:expiration', val)"
                >
                    <UiInput
                        :model-value="settings.expiration.label"
                        label="Expiration"
                        type="string"
                        :disabled="disabled"
                        readonly
                        wfull
                        cursor-pointer="!"
                    />
                </ExpirationPicker>

                <FolderPicker
                    :model-value="settings.folder"
                    @update:model-value="(val) => emit('update:folder', val)"
                >
                    <UiInput
                        :model-value="settings.folder.label"
                        label="Folder"
                        type="string"
                        :disabled="disabled"
                        readonly
                        wfull
                        cursor-pointer="!"
                    />
                </FolderPicker>
            </div>
        </div>

        <div v-if="showCompression" space-y-3>
            <h4 text-sm>Media Options</h4>
            <CompressionPicker
                :model-value="settings.compression"
                @update:model-value="(val) => emit('update:compression', val)"
            >
                <UiInput
                    :model-value="settings.compression.label"
                    label="Compression"
                    type="string"
                    :disabled="disabled"
                    readonly
                    wfull
                    cursor-pointer="!"
                />
            </CompressionPicker>
        </div>
    </div>
</template>

<script setup lang="ts">
type FileNameType = 'Random' | 'UUID' | 'Original';

interface ExpirationSetting {
    label: string;
    value: number | null;
}

interface CompressionSetting {
    label: string;
    value: number;
}

interface FolderSetting {
    label: string;
    value: string | null;
}

interface Settings {
    fileNameType: FileNameType;
    maxViews: number;
    password: string | null;
    expiration: ExpirationSetting;
    compression: CompressionSetting;
    folder: FolderSetting;
}

defineProps<{
    settings: Settings;
    disabled?: boolean;
    showFileNameType?: boolean;
    showCompression?: boolean;
}>();

const emit = defineEmits<{
    'update:fileNameType': [FileNameType];
    'update:password': [string | null];
    'update:maxViews': [number];
    'update:expiration': [{ label: string; value: number | null }];
    'update:folder': [{ label: string; value: string | null }];
    'update:compression': [{ label: string; value: number }];
}>();

const id = useId();
</script>
