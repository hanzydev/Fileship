<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>Upload options</UiCardTitle>
            <UiCardDescription>
                Used for screenshots, recordings and files uploaded from this computer.
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiFieldGroup v-if="upload">
                <UiField>
                    <UiFieldLabel>Folder</UiFieldLabel>
                    <UiSelect
                        :model-value="upload.folderId ?? NO_FOLDER"
                        @update:model-value="
                            updateSection('upload', {
                                folderId: $event === NO_FOLDER ? null : ($event as string),
                            })
                        "
                    >
                        <UiSelectTrigger class="w-full">
                            <UiSelectValue />
                        </UiSelectTrigger>
                        <UiSelectContent>
                            <UiSelectItem :value="NO_FOLDER">No folder</UiSelectItem>
                            <UiSelectItem
                                v-for="folder in folderOptions"
                                :key="folder.id"
                                :value="folder.id"
                            >
                                {{ folder.name }}
                            </UiSelectItem>
                        </UiSelectContent>
                    </UiSelect>
                </UiField>

                <UiField>
                    <UiFieldLabel>File name</UiFieldLabel>
                    <UiTabs
                        :model-value="upload.fileNameType"
                        @update:model-value="
                            updateSection('upload', { fileNameType: $event as FileNameType })
                        "
                    >
                        <UiTabsList class="w-full">
                            <UiTabsTrigger
                                v-for="type in FILE_NAME_TYPES"
                                :key="type"
                                :value="type"
                            >
                                {{ type }}
                            </UiTabsTrigger>
                        </UiTabsList>
                    </UiTabs>
                </UiField>

                <div class="grid grid-cols-1 gap-4 sm:grid-cols-2">
                    <UiField>
                        <UiFieldLabel>Expiration</UiFieldLabel>
                        <UiSelect
                            :model-value="upload.expiration"
                            @update:model-value="
                                updateSection('upload', { expiration: $event as number })
                            "
                        >
                            <UiSelectTrigger class="w-full">
                                <UiSelectValue />
                            </UiSelectTrigger>
                            <UiSelectContent>
                                <UiSelectItem
                                    v-for="option in EXPIRATIONS"
                                    :key="option.value"
                                    :value="option.value"
                                >
                                    {{ option.label }}
                                </UiSelectItem>
                            </UiSelectContent>
                        </UiSelect>
                    </UiField>

                    <UiField>
                        <UiFieldLabel>Compression</UiFieldLabel>
                        <UiSelect
                            :model-value="upload.compression"
                            @update:model-value="
                                updateSection('upload', { compression: $event as number })
                            "
                        >
                            <UiSelectTrigger class="w-full">
                                <UiSelectValue />
                            </UiSelectTrigger>
                            <UiSelectContent>
                                <UiSelectItem
                                    v-for="option in COMPRESSIONS"
                                    :key="option.value"
                                    :value="option.value"
                                >
                                    {{ option.label }}
                                </UiSelectItem>
                            </UiSelectContent>
                        </UiSelect>
                    </UiField>

                    <UiField>
                        <UiFieldLabel>Max views</UiFieldLabel>
                        <UiNumberField
                            :model-value="upload.maxViews"
                            :min="0"
                            @update:model-value="
                                updateSection('upload', { maxViews: Math.max(0, $event || 0) })
                            "
                        >
                            <UiNumberFieldContent>
                                <UiNumberFieldDecrement />
                                <UiNumberFieldInput />
                                <UiNumberFieldIncrement />
                            </UiNumberFieldContent>
                        </UiNumberField>
                        <UiFieldDescription>0 means unlimited.</UiFieldDescription>
                    </UiField>

                    <UiField>
                        <UiFieldLabel for="upload-password">Password</UiFieldLabel>
                        <PasswordInput
                            id="upload-password"
                            v-model="password"
                            placeholder="Optional"
                            maxlength="48"
                            @change="updateSection('upload', { password })"
                        />
                    </UiField>
                </div>
            </UiFieldGroup>
        </UiCardContent>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>Saved files</UiCardTitle>
            <UiCardDescription>
                Where captures go when saving to this computer is on in the capture settings.
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiFieldGroup v-if="save">
                <UiField>
                    <UiFieldLabel for="save-folder">Folder</UiFieldLabel>
                    <UiInputGroup>
                        <UiInputGroupInput
                            id="save-folder"
                            :model-value="save.folder"
                            placeholder="Pictures\Fileship"
                            readonly
                        />
                        <UiInputGroupAddon align="inline-end">
                            <UiInputGroupButton v-if="save.folder" @click="handleResetFolder">
                                Reset
                            </UiInputGroupButton>
                            <UiInputGroupButton @click="handlePickFolder">
                                Browse
                            </UiInputGroupButton>
                        </UiInputGroupAddon>
                    </UiInputGroup>
                </UiField>

                <UiField>
                    <UiFieldLabel for="save-pattern">File name pattern</UiFieldLabel>
                    <UiInput
                        id="save-pattern"
                        v-model="pattern"
                        maxlength="128"
                        @change="handlePattern"
                    />
                    <UiFieldDescription>
                        {{ preview }}.png · %yyyy year, %mo month, %d day, %h hour, %mi minute, %s
                        second, %ms millisecond, %pn window title, %ra random
                    </UiFieldDescription>
                </UiField>
            </UiFieldGroup>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
import type { Folder } from '../../../shared/ipc';
import { COMPRESSIONS, EXPIRATIONS, FILE_NAME_TYPES } from '../../../shared/settings';

type FileNameType = (typeof FILE_NAME_TYPES)[number];

const NO_FOLDER = 'none';
const DEFAULT_PATTERN = '%yyyy-%mo-%d_%h-%mi-%s';

const settings = useSettings();
const { $toast } = useNuxtApp();

const folders = ref<Folder[]>([]);
const password = ref(settings.value?.upload.password ?? '');
const pattern = ref(settings.value?.save.pattern ?? DEFAULT_PATTERN);

const upload = computed(() => settings.value?.upload);
const save = computed(() => settings.value?.save);

const folderOptions = computed(() => {
    const current = upload.value?.folderId;
    if (!current || folders.value.some((folder) => folder.id === current)) return folders.value;
    return [...folders.value, { id: current, name: 'Unknown folder' }];
});

const preview = computed(() => {
    const now = new Date();
    const pad = (value: number) => String(value).padStart(2, '0');

    return (pattern.value || DEFAULT_PATTERN)
        .replaceAll('%yyyy', String(now.getFullYear()))
        .replaceAll('%mo', pad(now.getMonth() + 1))
        .replaceAll('%d', pad(now.getDate()))
        .replaceAll('%h', pad(now.getHours()))
        .replaceAll('%mi', pad(now.getMinutes()))
        .replaceAll('%ms', String(now.getMilliseconds()).padStart(3, '0'))
        .replaceAll('%s', pad(now.getSeconds()))
        .replaceAll('%pn', 'Window')
        .replaceAll('%ra', 'a1b2c3d4e5')
        .replace(/[\\/:*?"<>|]/g, '');
});

const handlePattern = () => {
    if (!pattern.value.trim()) pattern.value = DEFAULT_PATTERN;
    updateSection('save', { pattern: pattern.value });
};

const handlePickFolder = async () => {
    const folder = await desktop('dialog:pickFolder');
    if (folder) updateSection('save', { folder });
};

const handleResetFolder = () => updateSection('save', { folder: '' });

onMounted(async () => {
    try {
        folders.value = await desktop('folders:list');
    } catch (error: any) {
        $toast.error(error.message);
    }
});
</script>
