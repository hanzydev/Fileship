<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('uploadSettings.title') }}</UiCardTitle>
            <UiCardDescription>{{ $t('uploadSettings.description') }}</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiFieldGroup v-if="upload">
                <UiField>
                    <UiFieldLabel>{{ $t('uploadSettings.folder') }}</UiFieldLabel>
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
                            <UiSelectItem :value="NO_FOLDER">
                                {{ $t('uploadSettings.noFolder') }}
                            </UiSelectItem>
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
                    <UiFieldLabel>{{ $t('uploadSettings.fileName') }}</UiFieldLabel>
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
                                {{ $t(`uploadSettings.fileNameTypes.${type}`) }}
                            </UiTabsTrigger>
                        </UiTabsList>
                    </UiTabs>
                </UiField>

                <div class="grid grid-cols-1 gap-4 sm:grid-cols-2">
                    <UiField>
                        <UiFieldLabel>{{ $t('uploadSettings.expiration') }}</UiFieldLabel>
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
                                    {{ $t(option.label) }}
                                </UiSelectItem>
                            </UiSelectContent>
                        </UiSelect>
                    </UiField>

                    <UiField>
                        <UiFieldLabel>{{ $t('uploadSettings.compression') }}</UiFieldLabel>
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
                                    {{ $t(option.label) }}
                                </UiSelectItem>
                            </UiSelectContent>
                        </UiSelect>
                    </UiField>

                    <UiField>
                        <UiFieldLabel>{{ $t('uploadSettings.maxViews') }}</UiFieldLabel>
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
                        <UiFieldDescription>
                            {{ $t('uploadSettings.maxViewsDescription') }}
                        </UiFieldDescription>
                    </UiField>

                    <UiField>
                        <UiFieldLabel for="upload-password">
                            {{ $t('uploadSettings.password') }}
                        </UiFieldLabel>
                        <PasswordInput
                            id="upload-password"
                            v-model="password"
                            :placeholder="$t('uploadSettings.optional')"
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
            <UiCardTitle>{{ $t('uploadSettings.savedFiles') }}</UiCardTitle>
            <UiCardDescription>{{ $t('uploadSettings.savedFilesDescription') }}</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiFieldGroup v-if="save">
                <UiField>
                    <UiFieldLabel for="save-folder">{{ $t('uploadSettings.folder') }}</UiFieldLabel>
                    <UiInputGroup>
                        <UiInputGroupInput
                            id="save-folder"
                            :model-value="save.folder"
                            :placeholder="$t('uploadSettings.saveFolderPlaceholder')"
                            readonly
                        />
                        <UiInputGroupAddon align="inline-end">
                            <UiInputGroupButton
                                v-if="save.folder"
                                :disabled="isPicking"
                                @click="handleResetFolder"
                            >
                                {{ $t('uploadSettings.reset') }}
                            </UiInputGroupButton>
                            <UiInputGroupButton :disabled="isPicking" @click="handlePickFolder">
                                {{ $t('uploadSettings.browse') }}
                            </UiInputGroupButton>
                        </UiInputGroupAddon>
                    </UiInputGroup>
                </UiField>

                <UiField>
                    <UiFieldLabel for="save-pattern">
                        {{ $t('uploadSettings.pattern') }}
                    </UiFieldLabel>
                    <UiInput
                        id="save-pattern"
                        v-model="pattern"
                        maxlength="128"
                        @change="handlePattern"
                    />
                    <UiFieldDescription>
                        {{ $t('uploadSettings.patternDescription', { preview }) }}
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
const { t } = useI18n();

const folders = ref<Folder[]>([]);
const isPicking = ref(false);
const password = ref(settings.value?.upload.password ?? '');
const pattern = ref(settings.value?.save.pattern ?? DEFAULT_PATTERN);

const upload = computed(() => settings.value?.upload);
const save = computed(() => settings.value?.save);

const folderOptions = computed(() => {
    const current = upload.value?.folderId;
    if (!current || folders.value.some((folder) => folder.id === current)) return folders.value;
    return [...folders.value, { id: current, name: t('uploadSettings.unknownFolder') }];
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
        .replaceAll('%pn', t('uploadSettings.sampleWindow'))
        .replaceAll('%ra', 'a1b2c3d4e5')
        .replace(/[\\/:*?"<>|]/g, '');
});

const handlePattern = () => {
    if (!pattern.value.trim()) pattern.value = DEFAULT_PATTERN;
    updateSection('save', { pattern: pattern.value });
};

const handleFolder = async (pick: () => Promise<string | null>) => {
    isPicking.value = true;

    try {
        const folder = await pick();
        if (folder !== null) await updateSection('save', { folder });
    } catch (error: any) {
        $toast.error(error.message);
    } finally {
        isPicking.value = false;
    }
};

const handlePickFolder = () => handleFolder(() => desktop('dialog:pickFolder'));

const handleResetFolder = () => handleFolder(async () => '');

onMounted(async () => {
    try {
        folders.value = await desktop('folders:list');
    } catch (error: any) {
        $toast.error(error.message);
    }
});
</script>
