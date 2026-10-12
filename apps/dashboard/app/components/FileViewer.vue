<template>
    <UiDialog :open="!!currentFile" @update:open="!$event && (fileId = null)">
        <UiDialogContent
            v-if="file"
            :show-close-button="false"
            class="top-0 left-0 grid h-svh w-screen max-w-none translate-x-0 translate-y-0 grid-rows-[45svh_minmax(0,1fr)] gap-0 overflow-hidden rounded-none bg-background p-0 ring-0 sm:max-w-none lg:grid-cols-[minmax(0,1fr)_24rem] lg:grid-rows-1"
            @open-auto-focus.prevent="closeButton?.$el?.focus()"
        >
            <div
                ref="stage"
                class="dark relative flex min-h-0 min-w-0 items-center justify-center overflow-hidden bg-background px-2 pt-2 pb-10 text-foreground lg:px-16 lg:py-10"
            >
                <div
                    v-if="isImage"
                    :class="
                        cn(
                            'flex size-full items-center justify-center overflow-hidden',
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
                                'max-h-full max-w-full object-contain transition-[scale,filter] duration-300',
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
                    class="max-h-full max-w-full"
                />
                <div v-else-if="isAudio" class="flex w-full max-w-md flex-col items-center gap-6">
                    <IMusic class="size-12 text-muted-foreground" />
                    <audio :key="file.id" :src="file.directUrl" controls class="w-full" />
                </div>
                <div v-else class="flex flex-col items-center gap-4">
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
                        v-if="prev"
                        variant="outline"
                        size="icon-sm"
                        class="absolute top-1/2 left-3 -translate-y-1/2"
                        aria-label="Previous file"
                        @click="fileId = prev.id"
                    >
                        <IChevronLeft />
                    </UiButton>
                    <UiButton
                        v-if="hasNext"
                        variant="outline"
                        size="icon-sm"
                        class="absolute top-1/2 right-3 -translate-y-1/2"
                        aria-label="Next file"
                        @click="goNext"
                    >
                        <IChevronRight />
                    </UiButton>
                    <UiBadge
                        v-if="index > -1"
                        variant="secondary"
                        class="absolute bottom-3 left-1/2 -translate-x-1/2 tabular-nums"
                    >
                        {{ index + 1 }} / {{ total ?? files.length }}
                    </UiBadge>
                </template>
            </div>

            <div class="flex min-h-0 flex-col border-t lg:border-t-0 lg:border-l">
                <div class="flex items-start gap-3 p-6 pb-4">
                    <UiDialogHeader class="min-w-0 flex-1">
                        <UiDialogTitle class="truncate">{{ file.fileName }}</UiDialogTitle>
                        <UiDialogDescription>
                            {{ file.size.formatted }} ·
                            {{ dayjs(file.createdAt).format('MMM D, YYYY HH:mm') }} ·
                            {{ pluralize(file.views.total, 'view') }}
                        </UiDialogDescription>
                    </UiDialogHeader>
                    <UiDialogClose as-child>
                        <UiButton
                            ref="closeButton"
                            variant="ghost"
                            size="icon-sm"
                            aria-label="Close"
                        >
                            <IX />
                        </UiButton>
                    </UiDialogClose>
                </div>

                <div class="flex gap-2 px-6 pb-4">
                    <UiButton class="flex-1" @click="copyLink(file)">
                        <ILink />
                        Copy link
                    </UiButton>
                    <UiTooltip v-for="action in footerActions" :key="action.label">
                        <UiTooltipTrigger as-child>
                            <UiButton
                                :variant="action.destructive ? 'destructive' : 'outline'"
                                size="icon"
                                :aria-label="action.label"
                                @click="action.run"
                            >
                                <component :is="action.icon" />
                            </UiButton>
                        </UiTooltipTrigger>
                        <UiTooltipContent>{{ action.label }}</UiTooltipContent>
                    </UiTooltip>
                </div>

                <div class="min-h-0 flex-1 overflow-y-auto px-6 pb-6">
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
                                        <UiFieldLabel for="edit-file-password">
                                            Password
                                        </UiFieldLabel>
                                        <PasswordInput
                                            id="edit-file-password"
                                            v-model="editData.password"
                                            placeholder="No password"
                                        />
                                        <UiFieldError
                                            v-if="formErrors?.password"
                                            :errors="formErrors.password._errors"
                                        />
                                    </UiField>
                                    <UiButton type="submit" class="self-end" :disabled="isSaving">
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
            </div>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

import { IDownload, IExternalLink, IFolderMinus, ITrash2 } from '#components';

const { files, total, hasMore, loadMore } = defineProps<{
    files: FileData[];
    total?: number;
    hasMore?: boolean;
    loadMore?: () => Promise<unknown>;
}>();

const fileId = defineModel<string | null>('fileId', { required: true });
const tab = defineModel<string>('tab', { default: 'details' });

const currentUser = useAuthUser();
const publicSettings = usePublicSettings();
const { $toast } = useNuxtApp();
const { openFile, copyLink, download, move, remove } = useFileActions();

const stage = useTemplateRef('stage');
const closeButton = useTemplateRef<any>('closeButton');

const isZoomed = ref(false);
const isRevealed = ref(false);
const isSaving = ref(false);
const zoomOrigin = ref('center');
const formErrors = ref();

const fallbackFile = ref<FileData>();

const currentFile = computed(
    () =>
        files.find((f) => f.id === fileId.value) ||
        (fallbackFile.value?.id === fileId.value ? fallbackFile.value : undefined),
);
const file = ref<FileData>();
const index = computed(() => files.findIndex((f) => f.id === fileId.value));
const prev = computed(() => files[index.value - 1]);
const next = computed(() => files[index.value + 1]);
const hasNext = computed(() => !!next.value || (index.value > -1 && hasMore));

const goNext = async () => {
    if (!next.value && hasMore) await loadMore?.();

    const target = files[index.value + 1];
    if (target) fileId.value = target.id;
};

const footerActions = computed(() =>
    file.value
        ? [
              { label: 'Open', icon: IExternalLink, run: () => openFile(file.value!) },
              { label: 'Download', icon: IDownload, run: () => download([file.value!]) },
              ...(file.value.folderId
                  ? [
                        {
                            label: 'Remove from folder',
                            icon: IFolderMinus,
                            run: () => move([file.value!], null),
                        },
                    ]
                  : []),
              {
                  label: 'Delete',
                  icon: ITrash2,
                  destructive: true,
                  run: () => remove([file.value!]),
              },
          ]
        : [],
);

const isImage = computed(() => file.value!.mimeType.startsWith('image/'));
const isVideo = computed(() => file.value!.mimeType.startsWith('video/'));
const isAudio = computed(() => file.value!.mimeType.startsWith('audio/'));

const isSensitive = computed(
    () =>
        isImage.value &&
        file.value!.piiDetected &&
        currentUser.value!.aiSettings.suppressPii &&
        publicSettings.value.aiEnabled,
);

const hasInsights = computed(
    () => publicSettings.value.aiEnabled && (!!file.value!.caption || file.value!.piiDetected),
);

const details = computed(() => {
    const f = file.value!;

    return [
        { label: 'Type', value: f.mimeType },
        { label: 'Size', value: f.size.formatted },
        { label: 'Folder', value: f.folder?.name || 'None' },
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
    const f = currentFile.value!;

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
        formErrors.value = error.data?.data?.formErrors;
        if (!formErrors.value) $toast.error(getErrorMessage(error));
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

watch(
    fileId,
    async (value) => {
        isZoomed.value = false;
        isRevealed.value = false;

        if (value && currentFile.value) resetEditData();

        if (value && !currentFile.value) {
            const result = await $fetch('/api/files', { query: { ids: value, limit: 1 } }).catch(
                () => null,
            );

            if (fileId.value !== value) return;

            if (result?.items[0]) fallbackFile.value = reviveDates(result.items[0]);
            else fileId.value = null;
        }
    },
    { immediate: true },
);

watch(
    currentFile,
    (value, oldValue) => {
        if (value) file.value = value;
        else if (fileId.value && oldValue) fileId.value = null;
    },
    { immediate: true },
);

watch(
    () => currentFile.value && tab.value === 'edit',
    (value) => value && resetEditData(),
    { immediate: true },
);

useSwipe(stage, {
    onSwipeEnd: (_, direction) => {
        if (isZoomed.value) return;
        if (direction === 'left') goNext();
        if (direction === 'right' && prev.value) fileId.value = prev.value.id;
    },
});

onKeyStroke(['ArrowLeft', 'ArrowRight'], (event) => {
    if (!currentFile.value || document.activeElement?.closest('input, textarea, video, audio'))
        return;

    if (event.key === 'ArrowRight') return goNext();
    if (prev.value) fileId.value = prev.value.id;
});

useListHandle({
    resource: 'files',
    refresh: () => null,
    map: (fn) => fallbackFile.value && (fallbackFile.value = fn(fallbackFile.value)),
    remove: (ids) =>
        fallbackFile.value &&
        ids.includes(fallbackFile.value.id) &&
        (fallbackFile.value = undefined),
});
</script>
