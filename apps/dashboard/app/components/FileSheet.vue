<template>
    <UiSheet :open="!!file" @update:open="!$event && (fileId = null)">
        <UiSheetContent v-if="file" class="data-[side=right]:w-full data-[side=right]:sm:max-w-2xl">
            <UiSheetHeader>
                <UiSheetTitle class="truncate pr-10">{{ file.fileName }}</UiSheetTitle>
                <UiSheetDescription>
                    {{ file.size.formatted }} ·
                    {{ dayjs(file.createdAt).format('MMM D, YYYY HH:mm') }} ·
                    {{ file.views.total }} views
                </UiSheetDescription>
            </UiSheetHeader>

            <div class="flex flex-1 flex-col gap-6 overflow-y-auto px-6">
                <div class="relative overflow-hidden rounded-2xl bg-muted">
                    <div
                        v-if="isImage"
                        :class="
                            cn(
                                'flex max-h-[60vh] min-h-48 items-center justify-center overflow-hidden',
                                isZoomed ? 'cursor-zoom-out' : 'cursor-zoom-in',
                            )
                        "
                        @click="handleZoom"
                    >
                        <img
                            :src="file.directUrl"
                            :alt="file.fileName"
                            :style="{ transformOrigin: zoomOrigin }"
                            :class="
                                cn(
                                    'max-h-[60vh] object-contain transition-[transform,filter] duration-300',
                                    isZoomed && 'scale-200',
                                    isSensitive && !isRevealed && 'blur-2xl',
                                )
                            "
                        />
                    </div>
                    <video
                        v-else-if="isVideo"
                        :key="file.id"
                        :src="file.directUrl"
                        :poster="file.thumbnailUrl || undefined"
                        controls
                        class="max-h-[60vh] w-full"
                    />
                    <div v-else-if="isAudio" class="flex flex-col items-center gap-4 p-10">
                        <IMusic class="size-10 text-muted-foreground" />
                        <audio :key="file.id" :src="file.directUrl" controls class="w-full" />
                    </div>
                    <div v-else class="flex flex-col items-center gap-3 p-12">
                        <component :is="getFileIcon(file)" class="size-12 text-muted-foreground" />
                        <UiButton variant="outline" @click="openFile(file)">
                            <IExternalLink />
                            Open in new tab
                        </UiButton>
                    </div>

                    <div
                        v-if="isSensitive && !isRevealed"
                        class="absolute inset-0 flex flex-col items-center justify-center gap-2"
                    >
                        <IShieldAlert class="size-8 text-destructive" />
                        <TextBlock variant="small">Sensitive content detected</TextBlock>
                        <UiButton variant="secondary" size="sm" @click="isRevealed = true">
                            Reveal
                        </UiButton>
                    </div>

                    <template v-if="files.length > 1">
                        <UiButton
                            variant="secondary"
                            size="icon-sm"
                            class="absolute top-1/2 left-2 -translate-y-1/2"
                            :disabled="!prev"
                            @click="fileId = prev!.id"
                        >
                            <IChevronLeft />
                        </UiButton>
                        <UiButton
                            variant="secondary"
                            size="icon-sm"
                            class="absolute top-1/2 right-2 -translate-y-1/2"
                            :disabled="!next"
                            @click="fileId = next!.id"
                        >
                            <IChevronRight />
                        </UiButton>
                    </template>
                </div>

                <UiTabs v-model="tab">
                    <UiTabsList class="w-full">
                        <UiTabsTrigger value="details">Details</UiTabsTrigger>
                        <UiTabsTrigger value="edit">Edit</UiTabsTrigger>
                        <UiTabsTrigger v-if="hasInsights" value="insights">
                            <ISparkles />
                            Insights
                        </UiTabsTrigger>
                    </UiTabsList>

                    <UiTabsContent value="details">
                        <dl class="grid grid-cols-[8rem_1fr] gap-x-4 gap-y-3 py-2 text-sm">
                            <template v-for="detail in details" :key="detail.label">
                                <dt class="text-muted-foreground">{{ detail.label }}</dt>
                                <dd class="truncate">{{ detail.value }}</dd>
                            </template>
                        </dl>
                    </UiTabsContent>

                    <UiTabsContent value="edit">
                        <form class="py-2" @submit.prevent="handleSave">
                            <UiFieldGroup>
                                <UiField :data-invalid="!!formErrors?.fileName">
                                    <UiFieldLabel for="edit-file-name">File name</UiFieldLabel>
                                    <UiInputGroup>
                                        <UiInputGroupInput
                                            id="edit-file-name"
                                            v-model="editData.fileName"
                                        />
                                        <UiInputGroupAddon align="inline-end">
                                            {{ getExtname(file.fileName) }}
                                        </UiInputGroupAddon>
                                    </UiInputGroup>
                                    <UiFieldError
                                        v-if="formErrors?.fileName"
                                        :errors="formErrors.fileName._errors"
                                    />
                                </UiField>
                                <UiField>
                                    <UiFieldLabel>Folder</UiFieldLabel>
                                    <FolderPicker v-model="editData.folderId" />
                                </UiField>
                                <div class="grid grid-cols-1 gap-4 sm:grid-cols-2">
                                    <UiField>
                                        <UiFieldLabel>Expiration</UiFieldLabel>
                                        <UiSelect v-model="editData.expiration">
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
                                    <UiField>
                                        <UiFieldLabel>Max views</UiFieldLabel>
                                        <UiNumberField v-model="editData.maxViews" :min="0">
                                            <UiNumberFieldContent>
                                                <UiNumberFieldDecrement />
                                                <UiNumberFieldInput />
                                                <UiNumberFieldIncrement />
                                            </UiNumberFieldContent>
                                        </UiNumberField>
                                    </UiField>
                                </div>
                                <UiField :data-invalid="!!formErrors?.password">
                                    <UiFieldLabel for="edit-file-password">Password</UiFieldLabel>
                                    <UiInput
                                        id="edit-file-password"
                                        v-model="editData.password"
                                        type="password"
                                        placeholder="No password"
                                    />
                                    <UiFieldError
                                        v-if="formErrors?.password"
                                        :errors="formErrors.password._errors"
                                    />
                                </UiField>
                                <UiButton type="submit" :disabled="isSaving">
                                    <UiSpinner v-if="isSaving" />
                                    Save changes
                                </UiButton>
                            </UiFieldGroup>
                        </form>
                    </UiTabsContent>

                    <UiTabsContent v-if="hasInsights" value="insights">
                        <div class="flex flex-col gap-4 py-2">
                            <div v-if="file.caption" class="flex flex-col gap-1">
                                <TextBlock variant="small">Caption</TextBlock>
                                <TextBlock variant="muted">{{ file.caption }}</TextBlock>
                            </div>
                            <UiAlert v-if="file.piiDetected" variant="destructive">
                                <IShieldAlert />
                                <UiAlertTitle>Sensitive information detected</UiAlertTitle>
                                <UiAlertDescription>
                                    <ul class="list-disc pl-4">
                                        <li v-for="reason in file.piiReasons" :key="reason">
                                            {{ reason }}
                                        </li>
                                    </ul>
                                </UiAlertDescription>
                            </UiAlert>
                        </div>
                    </UiTabsContent>
                </UiTabs>
            </div>

            <UiSheetFooter class="flex-row flex-wrap">
                <UiButton class="flex-1" @click="copyLink(file)">
                    <ILink />
                    Copy link
                </UiButton>
                <UiButton variant="outline" size="icon" @click="openFile(file)">
                    <IExternalLink />
                </UiButton>
                <UiButton variant="outline" size="icon" @click="download([file])">
                    <IDownload />
                </UiButton>
                <UiButton
                    v-if="file.folderId"
                    variant="outline"
                    size="icon"
                    @click="move([file], null)"
                >
                    <IFolderMinus />
                </UiButton>
                <UiButton variant="destructive" size="icon" @click="remove([file])">
                    <ITrash2 />
                </UiButton>
            </UiSheetFooter>
        </UiSheetContent>
    </UiSheet>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

const { files } = defineProps<{
    files: FileData[];
}>();

const fileId = defineModel<string | null>('fileId', { required: true });
const tab = defineModel<string>('tab', { default: 'details' });

const allFiles = useFiles();
const folders = useFolders();
const currentUser = useAuthUser();
const runtimeConfig = useRuntimeConfig();
const { $toast } = useNuxtApp();
const { openFile, copyLink, download, move, remove } = useFileActions();

const isZoomed = ref(false);
const isRevealed = ref(false);
const isSaving = ref(false);
const zoomOrigin = ref('center');
const formErrors = ref();

const file = computed(() => allFiles.value.find((f) => f.id === fileId.value));
const index = computed(() => files.findIndex((f) => f.id === fileId.value));
const prev = computed(() => files[index.value - 1]);
const next = computed(() => files[index.value + 1]);

const isImage = computed(() => file.value!.mimeType.startsWith('image/'));
const isVideo = computed(() => file.value!.mimeType.startsWith('video/'));
const isAudio = computed(() => file.value!.mimeType.startsWith('audio/'));

const isSensitive = computed(
    () =>
        isImage.value &&
        file.value!.piiDetected &&
        currentUser.value!.aiSettings.suppressPii &&
        runtimeConfig.public.aiEnabled,
);

const hasInsights = computed(
    () => runtimeConfig.public.aiEnabled && (!!file.value!.caption || file.value!.piiDetected),
);

const details = computed(() => {
    const f = file.value!;

    return [
        { label: 'Type', value: f.mimeType },
        { label: 'Size', value: f.size.formatted },
        { label: 'Folder', value: folders.value.find((x) => x.id === f.folderId)?.name || 'None' },
        { label: 'Views', value: `${f.views.total} total · ${f.views.today} today` },
        { label: 'Max views', value: f.maxViews || 'Unlimited' },
        { label: 'Password', value: f.password ? 'Protected' : 'None' },
        {
            label: 'Expires',
            value: f.expiresAt ? dayjs(f.expiresAt).format('MMM D, YYYY HH:mm') : 'Never',
        },
        { label: 'Uploaded', value: dayjs(f.createdAt).format('MMM D, YYYY HH:mm') },
    ];
});

const editData = reactive({
    fileName: '',
    folderId: null as string | null,
    expiration: 'Never',
    maxViews: 0,
    password: '',
});

const predictExpiration = (date: Date | null) => {
    if (!date) return 'Never';

    const diff = date.getTime() - Date.now();

    return EXPIRATIONS.slice(1).reduce((prev, curr) =>
        Math.abs(curr.value! - diff) < Math.abs(prev.value! - diff) ? curr : prev,
    ).label;
};

const resetEditData = () => {
    const f = file.value!;

    Object.assign(editData, {
        fileName: getBasename(f.fileName),
        folderId: f.folderId,
        expiration: predictExpiration(f.expiresAt),
        maxViews: f.maxViews,
        password: f.password || '',
    });

    formErrors.value = null;
};

const handleSave = async () => {
    const f = file.value!;
    const expiration = predictExpiration(f.expiresAt);

    isSaving.value = true;
    formErrors.value = null;

    try {
        await $fetch(`/api/files/${f.id}`, {
            method: 'PATCH',
            body: {
                fileName: `${editData.fileName}${getExtname(f.fileName)}`,
                folderId: editData.folderId,
                maxViews: editData.maxViews,
                password: editData.password || null,
                ...(editData.expiration !== expiration && {
                    expiration: EXPIRATIONS.find((e) => e.label === editData.expiration)!.value,
                }),
            },
        });

        $toast.success('File updated');
        tab.value = 'details';
    } catch (error: any) {
        formErrors.value = error.data.data?.formErrors;
        if (!formErrors.value) $toast.error(error.data.message);
    }

    isSaving.value = false;
};

const handleZoom = (event: MouseEvent) => {
    if (isSensitive.value && !isRevealed.value) return;

    if (!isZoomed.value) {
        const rect = (event.currentTarget as HTMLElement).getBoundingClientRect();
        zoomOrigin.value = `${event.clientX - rect.left}px ${event.clientY - rect.top}px`;
    }

    isZoomed.value = !isZoomed.value;
};

watch(fileId, (value) => {
    isZoomed.value = false;
    isRevealed.value = false;
    if (value && file.value) resetEditData();
});

watch(file, (value) => {
    if (fileId.value && !value) fileId.value = null;
});

watch(
    () => file.value && tab.value === 'edit',
    (value) => value && resetEditData(),
    { immediate: true },
);

onKeyStroke(['ArrowLeft', 'ArrowRight'], (event) => {
    if (!file.value || document.activeElement?.closest('input, textarea, video, audio')) return;

    const target = event.key === 'ArrowLeft' ? prev.value : next.value;
    if (target) fileId.value = target.id;
});
</script>
