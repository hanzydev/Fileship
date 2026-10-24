<template>
    <UiSheet v-model:open="open">
        <UiSheetContent class="data-[side=right]:w-full data-[side=right]:sm:max-w-lg">
            <UiSheetHeader>
                <UiSheetTitle>{{ $t('upload.title') }}</UiSheetTitle>
                <UiSheetDescription>{{ $t('upload.description') }}</UiSheetDescription>
            </UiSheetHeader>

            <div class="flex flex-1 flex-col gap-6 overflow-y-auto px-6">
                <UiTabs v-model="tab">
                    <UiTabsList class="w-full">
                        <UiTabsTrigger value="files">
                            <IImages />
                            {{ $t('upload.tabFiles') }}
                        </UiTabsTrigger>
                        <UiTabsTrigger value="text">
                            <IFileText />
                            {{ $t('upload.tabText') }}
                        </UiTabsTrigger>
                    </UiTabsList>

                    <UiTabsContent value="files" class="flex flex-col gap-3">
                        <DropZone :description="$t('upload.pasteHint')" @files="addFiles" />

                        <div v-if="files.length" class="flex flex-col gap-2">
                            <UiItem
                                v-for="(file, index) in files"
                                :key="`${file.name}-${index}`"
                                variant="muted"
                                size="sm"
                            >
                                <UiItemMedia variant="icon">
                                    <IFile />
                                </UiItemMedia>
                                <UiItemContent class="min-w-0">
                                    <UiItemTitle class="max-w-full">
                                        <span class="truncate">{{ file.name }}</span>
                                    </UiItemTitle>
                                    <UiItemDescription>
                                        {{ filesize(file.size, { locale }) }}
                                    </UiItemDescription>
                                </UiItemContent>
                                <UiItemActions>
                                    <UiButton
                                        variant="ghost"
                                        size="icon-sm"
                                        :aria-label="$t('upload.remove')"
                                        @click="files.splice(index, 1)"
                                    >
                                        <IX />
                                    </UiButton>
                                </UiItemActions>
                            </UiItem>
                        </div>
                    </UiTabsContent>

                    <UiTabsContent value="text">
                        <UiFieldGroup>
                            <div class="grid grid-cols-1 gap-4 sm:grid-cols-[1fr_10rem]">
                                <UiField>
                                    <UiFieldLabel for="text-file-name">
                                        {{ $t('upload.fileName') }}
                                    </UiFieldLabel>
                                    <UiInput
                                        id="text-file-name"
                                        v-model="text.fileName"
                                        :placeholder="$t('upload.fileNamePlaceholder')"
                                    />
                                </UiField>
                                <UiField>
                                    <UiFieldLabel>{{ $t('upload.type') }}</UiFieldLabel>
                                    <UiSelect v-model="text.extension">
                                        <UiSelectTrigger class="w-full">
                                            <UiSelectValue />
                                        </UiSelectTrigger>
                                        <UiSelectContent>
                                            <UiSelectItem
                                                v-for="type in TEXT_FILE_TYPES"
                                                :key="type.extension"
                                                :value="type.extension"
                                            >
                                                {{
                                                    type.extension === 'txt'
                                                        ? $t('upload.plainText')
                                                        : type.label
                                                }}
                                            </UiSelectItem>
                                        </UiSelectContent>
                                    </UiSelect>
                                </UiField>
                            </div>
                            <UiField>
                                <UiFieldLabel for="text-content">
                                    {{ $t('upload.content') }}
                                </UiFieldLabel>
                                <UiTextarea
                                    id="text-content"
                                    v-model="text.content"
                                    class="min-h-48 font-mono"
                                />
                            </UiField>
                        </UiFieldGroup>
                    </UiTabsContent>
                </UiTabs>

                <UiCollapsible v-model:open="optionsOpen">
                    <UiCollapsibleTrigger as-child>
                        <UiButton variant="ghost" class="w-full justify-between">
                            {{ $t('upload.options') }}
                            <IChevronDown
                                :class="cn('transition-transform', optionsOpen && 'rotate-180')"
                            />
                        </UiButton>
                    </UiCollapsibleTrigger>
                    <UiCollapsibleContent>
                        <UploadOptions
                            v-model:folder="settings.folder"
                            v-model:file-name-type="settings.fileNameType"
                            v-model:expiration="expiration"
                            v-model:compression="settings.compression"
                            v-model:max-views="settings.maxViews"
                            v-model:password="settings.password"
                            :text="tab === 'text'"
                            class="pt-4"
                        />
                    </UiCollapsibleContent>
                </UiCollapsible>
            </div>

            <UiSheetFooter>
                <UiButton :disabled="!canUpload" @click="handleUpload">
                    <IUpload />
                    {{ uploadLabel }}
                </UiButton>
                <UiSheetClose as-child>
                    <UiButton variant="outline">{{ $t('common.cancel') }}</UiButton>
                </UiSheetClose>
            </UiSheetFooter>
        </UiSheetContent>
    </UiSheet>
</template>

<script setup lang="ts">
import { filesize } from 'filesize';

const open = useUploadSheet();
const { currentFolder, upload, copyUrls } = useUploader();
const { $toast } = useNuxtApp();
const { t, locale } = useI18n();

const tab = ref<'files' | 'text'>('files');
const optionsOpen = ref(false);
const files = ref<File[]>([]);
const expiration = ref('labels.expiration.never');

const text = reactive({
    fileName: '',
    extension: 'txt',
    content: '',
});

const settings = reactive({
    fileNameType: 'Random' as NonNullable<FileSettings['fileNameType']>,
    maxViews: 0,
    password: '',
    compression: 0,
    folder: null as string | null,
});

const canUpload = computed(() =>
    tab.value === 'files' ? !!files.value.length : !!text.fileName.trim() && !!text.content,
);

const uploadLabel = computed(() =>
    tab.value === 'text' ? t('upload.createTextFile') : t('upload.uploadCount', files.value.length),
);

const addFiles = (newFiles: File[] | null) => {
    if (!newFiles?.length) return;

    tab.value = 'files';
    files.value = mergeFiles(files.value, newFiles);
};

const handleUpload = async () => {
    const isText = tab.value === 'text';
    const type = TEXT_FILE_TYPES.find((t) => t.extension === text.extension)!;

    const uploadFiles = isText
        ? [
              new File([text.content], `${text.fileName.trim()}.${type.extension}`, {
                  type: type.mime,
              }),
          ]
        : files.value;

    const uploadSettings = {
        ...settings,
        fileNameType: isText ? ('Original' as const) : settings.fileNameType,
        compression: isText ? 0 : settings.compression,
        password: settings.password || null,
        expiration: EXPIRATIONS.find((e) => e.label === expiration.value)!.value,
    };

    open.value = false;
    files.value = [];
    if (isText) Object.assign(text, { fileName: '', content: '' });

    const { urls, failed } = await upload(uploadFiles, uploadSettings);

    if (failed) $toast.error(t('upload.someFailed'));
    else await copyUrls(urls, t(isText ? 'upload.textCreated' : 'upload.uploadComplete'));
};

watch(open, (value) => {
    if (value) settings.folder = currentFolder.value;
});

defineExpose({ addFiles });
</script>
