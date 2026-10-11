<template>
    <UiSheet v-model:open="open">
        <UiSheetContent class="data-[side=right]:w-full data-[side=right]:sm:max-w-lg">
            <UiSheetHeader>
                <UiSheetTitle>Upload</UiSheetTitle>
                <UiSheetDescription>
                    Upload files or create a text file. You can keep working while it uploads.
                </UiSheetDescription>
            </UiSheetHeader>

            <div class="flex flex-1 flex-col gap-6 overflow-y-auto px-6">
                <UiTabs v-model="tab">
                    <UiTabsList class="w-full">
                        <UiTabsTrigger value="files">
                            <IImages />
                            Files
                        </UiTabsTrigger>
                        <UiTabsTrigger value="text">
                            <IFileText />
                            Text
                        </UiTabsTrigger>
                    </UiTabsList>

                    <UiTabsContent value="files" class="flex flex-col gap-3">
                        <label
                            ref="dropArea"
                            :class="
                                cn(
                                    'flex flex-col items-center justify-center gap-2 rounded-xl border-2 border-dashed p-8 text-center transition-colors',
                                    isOverDropArea
                                        ? 'border-primary bg-primary/5'
                                        : 'hover:bg-muted/50',
                                )
                            "
                        >
                            <IUpload class="size-6 text-muted-foreground" />
                            <TextBlock variant="small">
                                Drop files here or click to browse
                            </TextBlock>
                            <TextBlock variant="muted">
                                You can also paste images anywhere.
                            </TextBlock>
                            <input type="file" multiple class="sr-only" @change="handleInput" />
                        </label>

                        <div v-if="files.length" class="flex flex-col gap-2">
                            <UiCard
                                v-for="(file, index) in files"
                                :key="`${file.name}-${index}`"
                                size="sm"
                            >
                                <UiCardContent class="flex items-center gap-3">
                                    <IFile class="size-4 shrink-0 text-muted-foreground" />
                                    <TextBlock variant="small" class="flex-1 truncate">
                                        {{ file.name }}
                                    </TextBlock>
                                    <TextBlock variant="muted" class="shrink-0">
                                        {{ filesize(file.size) }}
                                    </TextBlock>
                                    <UiButton
                                        variant="ghost"
                                        size="icon-sm"
                                        @click="files.splice(index, 1)"
                                    >
                                        <IX />
                                    </UiButton>
                                </UiCardContent>
                            </UiCard>
                        </div>
                    </UiTabsContent>

                    <UiTabsContent value="text">
                        <UiFieldGroup>
                            <div class="grid grid-cols-1 gap-4 sm:grid-cols-[1fr_10rem]">
                                <UiField>
                                    <UiFieldLabel for="text-file-name">File name</UiFieldLabel>
                                    <UiInput
                                        id="text-file-name"
                                        v-model="text.fileName"
                                        placeholder="notes"
                                    />
                                </UiField>
                                <UiField>
                                    <UiFieldLabel>Type</UiFieldLabel>
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
                                                {{ type.label }}
                                            </UiSelectItem>
                                        </UiSelectContent>
                                    </UiSelect>
                                </UiField>
                            </div>
                            <UiField>
                                <UiFieldLabel for="text-content">Content</UiFieldLabel>
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
                            Options
                            <IChevronDown
                                :class="cn('transition-transform', optionsOpen && 'rotate-180')"
                            />
                        </UiButton>
                    </UiCollapsibleTrigger>
                    <UiCollapsibleContent>
                        <UiFieldGroup class="pt-4">
                            <UiField>
                                <UiFieldLabel>Folder</UiFieldLabel>
                                <FolderPicker v-model="settings.folder" />
                            </UiField>

                            <UiField v-if="tab === 'files'">
                                <UiFieldLabel>File name</UiFieldLabel>
                                <UiTabs v-model="settings.fileNameType">
                                    <UiTabsList class="w-full">
                                        <UiTabsTrigger
                                            v-for="type in fileNameTypes"
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
                                                {{ option.label }}
                                            </UiSelectItem>
                                        </UiSelectContent>
                                    </UiSelect>
                                </UiField>

                                <UiField v-if="tab === 'files'">
                                    <UiFieldLabel>Compression</UiFieldLabel>
                                    <UiSelect v-model="settings.compression">
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
                                    <UiNumberField v-model="settings.maxViews" :min="0">
                                        <UiNumberFieldContent>
                                            <UiNumberFieldDecrement />
                                            <UiNumberFieldInput />
                                            <UiNumberFieldIncrement />
                                        </UiNumberFieldContent>
                                    </UiNumberField>
                                    <UiFieldDescription>0 means unlimited.</UiFieldDescription>
                                </UiField>

                                <UiField :class="tab === 'text' && 'sm:col-span-2'">
                                    <UiFieldLabel for="upload-password">Password</UiFieldLabel>
                                    <UiInput
                                        id="upload-password"
                                        v-model="settings.password"
                                        type="password"
                                        placeholder="Optional"
                                    />
                                </UiField>
                            </div>
                        </UiFieldGroup>
                    </UiCollapsibleContent>
                </UiCollapsible>
            </div>

            <UiSheetFooter>
                <UiButton :disabled="!canUpload" @click="handleUpload">
                    <IUpload />
                    {{ uploadLabel }}
                </UiButton>
            </UiSheetFooter>
        </UiSheetContent>
    </UiSheet>
</template>

<script setup lang="ts">
import { filesize } from 'filesize';

const open = useUploadSheet();
const { currentFolder, upload, copyUrls } = useUploader();
const { $toast } = useNuxtApp();

const dropArea = useTemplateRef('dropArea');

const tab = ref<'files' | 'text'>('files');
const optionsOpen = ref(false);
const files = ref<File[]>([]);
const expiration = ref('Never');

const fileNameTypes = ['Random', 'UUID', 'Original'] as const;

const text = reactive({
    fileName: '',
    extension: 'txt',
    content: '',
});

const settings = reactive({
    fileNameType: 'Random' as (typeof fileNameTypes)[number],
    maxViews: 0,
    password: '',
    compression: 0,
    folder: null as string | null,
});

const canUpload = computed(() =>
    tab.value === 'files' ? !!files.value.length : !!text.fileName.trim() && !!text.content,
);

const uploadLabel = computed(() =>
    tab.value === 'text'
        ? 'Create text file'
        : files.value.length > 1
          ? `Upload ${files.value.length} files`
          : 'Upload',
);

const addFiles = (newFiles: File[] | null) => {
    if (!newFiles?.length) return;

    tab.value = 'files';
    files.value = [
        ...files.value,
        ...newFiles.filter((f) => !files.value.some((file) => file.name === f.name)),
    ];
};

const handleInput = (event: Event) => {
    const input = event.target as HTMLInputElement;
    addFiles(Array.from(input.files!));
    input.value = '';
};

const { isOverDropZone: isOverDropArea } = useDropZone(dropArea, {
    onDrop: addFiles,
    checkValidity: (items) => [...items].some((item) => item.kind === 'file'),
    multiple: true,
});

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

    if (failed) $toast.error('Some files could not be uploaded');
    else await copyUrls(urls, isText ? 'Text file created.' : 'Upload complete.');
};

watch(open, (value) => {
    if (value) settings.folder = currentFolder.value;
});

defineExpose({ addFiles });
</script>
