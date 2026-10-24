<template>
    <UiFieldGroup>
        <UiField>
            <UiFieldLabel>{{ $t('upload.folder') }}</UiFieldLabel>
            <FolderPicker v-model="folder" />
        </UiField>

        <UiField v-if="!text">
            <UiFieldLabel>{{ $t('upload.fileName') }}</UiFieldLabel>
            <UiTabs v-model="fileNameType">
                <UiTabsList class="w-full">
                    <UiTabsTrigger v-for="type in fileNameTypes" :key="type" :value="type">
                        {{ $t(`upload.fileNameType.${type.toLowerCase()}`) }}
                    </UiTabsTrigger>
                </UiTabsList>
            </UiTabs>
        </UiField>

        <div class="grid grid-cols-1 gap-4 sm:grid-cols-2">
            <UiField>
                <UiFieldLabel>{{ $t('upload.expiration') }}</UiFieldLabel>
                <UiSelect v-model="expiration">
                    <UiSelectTrigger class="w-full">
                        <UiSelectValue />
                    </UiSelectTrigger>
                    <UiSelectContent>
                        <UiSelectItem
                            v-for="option in EXPIRATIONS"
                            :key="option.label"
                            :value="option.label"
                        >
                            {{ $t(option.label) }}
                        </UiSelectItem>
                    </UiSelectContent>
                </UiSelect>
            </UiField>

            <UiField v-if="!text">
                <UiFieldLabel>{{ $t('upload.compression') }}</UiFieldLabel>
                <UiSelect v-model="compression">
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
                <UiFieldLabel>{{ $t('upload.maxViews') }}</UiFieldLabel>
                <UiNumberField v-model="maxViews" :min="0">
                    <UiNumberFieldContent>
                        <UiNumberFieldDecrement />
                        <UiNumberFieldInput />
                        <UiNumberFieldIncrement />
                    </UiNumberFieldContent>
                </UiNumberField>
                <UiFieldDescription>
                    {{ $t('upload.maxViewsDescription') }}
                </UiFieldDescription>
            </UiField>

            <UiField :class="text && 'sm:col-span-2'">
                <UiFieldLabel :for="passwordId">
                    {{ $t('upload.password') }}
                </UiFieldLabel>
                <PasswordInput
                    :id="passwordId"
                    v-model="password"
                    :placeholder="$t('upload.optional')"
                />
            </UiField>
        </div>
    </UiFieldGroup>
</template>

<script setup lang="ts">
defineProps<{ text?: boolean }>();

const folder = defineModel<string | null>('folder', { required: true });
const fileNameType = defineModel<NonNullable<FileSettings['fileNameType']>>('fileNameType', {
    required: true,
});
const expiration = defineModel<string>('expiration', { required: true });
const compression = defineModel<number>('compression', { required: true });
const maxViews = defineModel<number>('maxViews', { required: true });
const password = defineModel<string>('password', { required: true });

const passwordId = useId();

const fileNameTypes = ['Random', 'UUID', 'Original'] as const;
</script>
