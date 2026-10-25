<template>
    <UiDialog :open="!!currentFile" @update:open="!$event && (fileId = null)">
        <UiDialogContent
            v-if="file"
            :show-close-button="false"
            class="top-0 left-0 grid h-svh w-screen max-w-none translate-x-0 translate-y-0 grid-rows-[55svh_minmax(0,1fr)] gap-0 overflow-hidden rounded-none bg-background p-0 ring-0 sm:max-w-none lg:grid-cols-[minmax(0,1fr)_24rem] lg:grid-rows-1"
            @open-auto-focus.prevent="closeButton?.$el?.focus()"
        >
            <div
                class="dark flex min-h-0 min-w-0 flex-col overflow-hidden bg-background text-foreground"
            >
                <div
                    ref="stage"
                    class="relative flex min-h-0 flex-1 items-center justify-center overflow-hidden px-2 pt-10 pb-2 [container-type:size] lg:px-16 lg:py-10"
                    @click.self="handleBackdropClick"
                >
                    <div
                        v-if="isImage"
                        class="flex size-full items-center justify-center overflow-hidden"
                        @click.self="handleBackdropClick"
                    >
                        <UiSkeleton v-if="hasPlaceholder && !isLoaded" :style="mediaBoxStyle" />
                        <img
                            v-show="isLoaded || !hasPlaceholder"
                            :src="file.directUrl"
                            :alt="file.name"
                            draggable="false"
                            :style="{ transformOrigin: zoomOrigin }"
                            :class="
                                cn(
                                    'max-h-full max-w-full object-contain transition-[scale,filter] duration-300 select-none',
                                    isZoomed ? 'scale-250 cursor-zoom-out' : 'cursor-zoom-in',
                                    isSensitive && !isRevealed && 'blur-2xl',
                                )
                            "
                            @click="handleZoom"
                            @pointermove="isZoomed && setZoomOrigin($event)"
                            @load="loadedId = file.id"
                            @error="loadedId = file.id"
                        />
                    </div>
                    <template v-else-if="isVideo">
                        <UiSkeleton v-if="hasPlaceholder && !isLoaded" :style="mediaBoxStyle" />
                        <video
                            v-show="isLoaded || !hasPlaceholder"
                            :key="file.id"
                            :src="file.directUrl"
                            :poster="file.thumbnailUrl || undefined"
                            controls
                            class="max-h-full max-w-full"
                            :style="hasPlaceholder && mediaBoxStyle"
                            @loadedmetadata="loadedId = file.id"
                            @error="loadedId = file.id"
                        />
                    </template>
                    <div
                        v-else-if="isAudio"
                        class="flex w-full max-w-md flex-col items-center gap-6"
                    >
                        <IMusic class="size-12 text-muted-foreground" />
                        <audio :key="file.id" :src="file.directUrl" controls class="w-full" />
                    </div>
                    <PdfViewer
                        v-else-if="isPdfPreview"
                        :key="file.id"
                        :src="`/u/${encodeURIComponent(file.fileName)}`"
                        @error="failedPdfId = file.id"
                    />
                    <TextPreview
                        v-else-if="isTextPreview"
                        :key="file.id"
                        v-model:content="textContent"
                        :src="`/u/${encodeURIComponent(file.fileName)}`"
                        :name="file.name"
                        :mime-type="file.mimeType"
                        :size="Number(file.size.raw)"
                        :preview="textMode === 'preview'"
                        class="size-full overflow-auto rounded-2xl border bg-card p-4 text-card-foreground"
                    >
                        <template #too-large>
                            <UiButton variant="outline" @click="handleOpenFile">
                                <IExternalLink />
                                {{ $t('fileViewer.openInNewTab') }}
                            </UiButton>
                        </template>
                    </TextPreview>
                    <div v-else class="flex flex-col items-center gap-4">
                        <component :is="getFileIcon(file)" class="size-12 text-muted-foreground" />
                        <UiButton variant="outline" @click="handleOpenFile">
                            <IExternalLink />
                            {{ $t('fileViewer.openInNewTab') }}
                        </UiButton>
                    </div>

                    <div
                        v-if="isSensitive && !isRevealed"
                        class="absolute inset-0 flex flex-col items-center justify-center gap-2"
                    >
                        <IShieldAlert class="size-8 text-destructive" />
                        <TextBlock variant="small">
                            {{ $t('fileViewer.sensitiveContent') }}
                        </TextBlock>
                        <UiButton variant="secondary" size="sm" @click="isRevealed = true">
                            {{ $t('fileViewer.reveal') }}
                        </UiButton>
                    </div>

                    <div
                        v-if="isTextPreview && textContent !== undefined"
                        :class="cn('absolute top-2 right-3 flex items-center gap-2', overlayClass)"
                    >
                        <UiToggleGroup
                            v-if="hasRichPreview"
                            type="single"
                            variant="outline"
                            size="sm"
                            :model-value="textMode"
                            @update:model-value="handleTextMode"
                        >
                            <UiToggleGroupItem value="preview" :aria-label="$t('view.preview')">
                                <IEye />
                            </UiToggleGroupItem>
                            <UiToggleGroupItem value="raw" :aria-label="$t('view.raw')">
                                <ICode />
                            </UiToggleGroupItem>
                        </UiToggleGroup>
                        <UiButton
                            variant="outline"
                            size="icon-sm"
                            :aria-label="$t('view.copy')"
                            @click="handleCopyContent"
                        >
                            <ICopy />
                        </UiButton>
                    </div>

                    <template v-if="files.length > 1">
                        <UiButton
                            v-if="prev"
                            variant="outline"
                            size="icon-sm"
                            :class="cn('absolute inset-y-0 left-3 my-auto', overlayClass)"
                            :aria-label="$t('fileViewer.previousFile')"
                            @click="fileId = prev.id"
                        >
                            <IChevronLeft />
                        </UiButton>
                        <UiButton
                            v-if="hasNext"
                            variant="outline"
                            size="icon-sm"
                            :class="cn('absolute inset-y-0 right-3 my-auto', overlayClass)"
                            :aria-label="$t('fileViewer.nextFile')"
                            @click="goNext"
                        >
                            <IChevronRight />
                        </UiButton>
                        <UiBadge
                            v-if="index > -1"
                            variant="secondary"
                            :class="cn('absolute top-3 left-3 tabular-nums', overlayClass)"
                        >
                            {{ index + 1 }} / {{ total ?? files.length }}
                        </UiBadge>
                    </template>
                </div>

                <div
                    v-if="files.length > 1"
                    ref="strip"
                    :class="cn('no-scrollbar shrink-0 overflow-x-auto pt-2 pb-4', overlayClass)"
                    @wheel="handleStripWheel"
                >
                    <div
                        class="relative mx-auto h-12"
                        :style="{ width: `${stripVirtualizer.getTotalSize()}px` }"
                    >
                        <button
                            v-for="item in stripItems"
                            :key="item.file.id"
                            type="button"
                            :aria-label="item.file.name"
                            :class="
                                cn(
                                    'absolute top-0 left-0 flex size-12 items-center justify-center overflow-hidden rounded-md bg-muted transition-opacity',
                                    item.file.id === fileId
                                        ? 'ring-2 ring-primary'
                                        : 'opacity-50 hover:opacity-100',
                                )
                            "
                            :style="{ transform: `translateX(${item.start}px)` }"
                            @click="fileId = item.file.id"
                        >
                            <FileThumbnail :file="item.file" />
                        </button>
                    </div>
                </div>
            </div>

            <div class="flex min-h-0 flex-col border-t lg:border-t-0 lg:border-l">
                <div class="flex items-start gap-3 p-6 pb-4">
                    <UiDialogHeader class="min-w-0 flex-1">
                        <UiDialogTitle class="truncate">{{ file.name }}</UiDialogTitle>
                        <UiDialogDescription>
                            {{
                                $t('fileViewer.summary', {
                                    size: formatSize(file),
                                    date: date(file.createdAt).format('lll'),
                                    views: $t('fileViewer.viewCount', file.views.total),
                                })
                            }}
                        </UiDialogDescription>
                    </UiDialogHeader>
                    <UiDialogClose as-child>
                        <UiButton
                            ref="closeButton"
                            variant="ghost"
                            size="icon-sm"
                            :aria-label="$t('common.close')"
                        >
                            <IX />
                        </UiButton>
                    </UiDialogClose>
                </div>

                <div class="flex gap-2 px-6 pb-4">
                    <UiButton
                        v-if="trash"
                        class="flex-1"
                        :disabled="isRestoring"
                        @click="handleRestore"
                    >
                        <UiSpinner v-if="isRestoring" />
                        <IRotateCcw v-else />
                        {{ $t('fileViewer.restore') }}
                    </UiButton>
                    <UiButton v-else class="flex-1" @click="copyLink(file)">
                        <ILink />
                        {{ $t('fileViewer.copyLink') }}
                    </UiButton>
                    <UiTooltip v-for="action in footerActions" :key="action.label">
                        <UiTooltipTrigger as-child>
                            <UiButton
                                :variant="action.destructive ? 'destructive' : 'outline'"
                                size="icon"
                                :aria-label="action.label"
                                :disabled="!!runningAction"
                                @click="handleAction(action)"
                            >
                                <UiSpinner v-if="runningAction === action.label" />
                                <component :is="action.icon" v-else />
                            </UiButton>
                        </UiTooltipTrigger>
                        <UiTooltipContent>{{ action.label }}</UiTooltipContent>
                    </UiTooltip>
                </div>

                <div class="min-h-0 flex-1 overflow-y-auto px-6 pb-6">
                    <UiTabs v-model="activeTab">
                        <UiTabsList v-if="!trash || hasInsights" class="w-full">
                            <UiTabsTrigger value="details">
                                {{ $t('fileViewer.details') }}
                            </UiTabsTrigger>
                            <UiTabsTrigger v-if="!trash && isEditable" value="edit">
                                {{ $t('fileViewer.edit') }}
                            </UiTabsTrigger>
                            <UiTabsTrigger v-if="hasInsights" value="insights">
                                <ISparkles />
                                {{ $t('fileViewer.insights') }}
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

                        <UiTabsContent v-if="!trash && isEditable" value="edit">
                            <form class="py-2" @submit.prevent="handleSave">
                                <UiFieldGroup>
                                    <UiField :data-invalid="!!formErrors?.name">
                                        <UiFieldLabel for="edit-name">
                                            {{ $t('fileViewer.name') }}
                                        </UiFieldLabel>
                                        <UiInput id="edit-name" v-model="editData.name" />
                                        <UiFieldError
                                            v-if="formErrors?.name"
                                            :errors="formErrors.name._errors"
                                        />
                                    </UiField>
                                    <UiField v-if="isOwner" :data-invalid="!!formErrors?.fileName">
                                        <UiFieldLabel for="edit-file-name">
                                            {{ $t('fileViewer.linkName') }}
                                        </UiFieldLabel>
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
                                    <UiField v-if="isOwner">
                                        <UiFieldLabel>{{ $t('fileViewer.folder') }}</UiFieldLabel>
                                        <FolderPicker v-model="editData.folderId" />
                                    </UiField>
                                    <div
                                        v-if="isOwner"
                                        class="grid grid-cols-1 gap-4 sm:grid-cols-2"
                                    >
                                        <UiField>
                                            <UiFieldLabel>
                                                {{ $t('fileViewer.expiration') }}
                                            </UiFieldLabel>
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
                                                        {{ $t(option.label) }}
                                                    </UiSelectItem>
                                                </UiSelectContent>
                                            </UiSelect>
                                        </UiField>
                                        <UiField>
                                            <UiFieldLabel>
                                                {{ $t('fileViewer.maxViews') }}
                                            </UiFieldLabel>
                                            <UiNumberField v-model="editData.maxViews" :min="0">
                                                <UiNumberFieldContent>
                                                    <UiNumberFieldDecrement />
                                                    <UiNumberFieldInput />
                                                    <UiNumberFieldIncrement />
                                                </UiNumberFieldContent>
                                            </UiNumberField>
                                        </UiField>
                                    </div>
                                    <UiField v-if="isOwner" :data-invalid="!!formErrors?.password">
                                        <UiFieldLabel for="edit-file-password">
                                            {{ $t('fileViewer.password') }}
                                        </UiFieldLabel>
                                        <PasswordInput
                                            id="edit-file-password"
                                            v-model="editData.password"
                                            :placeholder="$t('fileViewer.noPassword')"
                                        />
                                        <UiFieldError
                                            v-if="formErrors?.password"
                                            :errors="formErrors.password._errors"
                                        />
                                    </UiField>
                                    <UiButton type="submit" class="self-end" :disabled="isSaving">
                                        <UiSpinner v-if="isSaving" />
                                        {{ $t('fileViewer.saveChanges') }}
                                    </UiButton>
                                </UiFieldGroup>
                            </form>
                        </UiTabsContent>

                        <UiTabsContent v-if="hasInsights" value="insights">
                            <div class="flex flex-col gap-4 py-2">
                                <div
                                    v-if="isAnalyzing"
                                    class="flex items-center gap-2 text-muted-foreground"
                                >
                                    <UiSpinner />
                                    <TextBlock variant="muted">
                                        {{ $t('fileViewer.analyzing') }}
                                    </TextBlock>
                                </div>
                                <div v-if="file.caption" class="flex flex-col gap-1">
                                    <TextBlock variant="small">
                                        {{ $t('fileViewer.caption') }}
                                    </TextBlock>
                                    <TextBlock variant="muted">{{ file.caption }}</TextBlock>
                                </div>
                                <div v-if="file.piiDetected" class="flex flex-col gap-2">
                                    <UiAlert variant="destructive">
                                        <IShieldAlert />
                                        <UiAlertTitle>
                                            {{
                                                file.piiOverride
                                                    ? $t('fileViewer.markedSensitive')
                                                    : $t('fileViewer.sensitiveDetected')
                                            }}
                                        </UiAlertTitle>
                                        <UiAlertDescription>
                                            <ul
                                                v-if="!file.piiOverride && file.piiReasons.length"
                                                class="list-disc pl-4"
                                            >
                                                <li v-for="reason in file.piiReasons" :key="reason">
                                                    {{
                                                        PII_REASONS[reason]
                                                            ? $t(PII_REASONS[reason])
                                                            : reason
                                                    }}
                                                </li>
                                            </ul>
                                            <p>{{ $t('fileViewer.hiddenFromPublic') }}</p>
                                        </UiAlertDescription>
                                    </UiAlert>
                                    <UiButton
                                        v-if="!trash && isOwner"
                                        variant="outline"
                                        size="sm"
                                        class="self-start"
                                        :disabled="isMarking"
                                        @click="handleNotSensitive"
                                    >
                                        <UiSpinner v-if="isMarking" />
                                        <IShieldCheck v-else />
                                        {{ $t('fileViewer.notSensitive') }}
                                    </UiButton>
                                </div>
                            </div>
                        </UiTabsContent>
                    </UiTabs>
                </div>
            </div>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
import { filesize } from 'filesize';

import { useVirtualizer } from '@tanstack/vue-virtual';

import { IDownload, IExternalLink, IFolderMinus, ITrash2 } from '#components';

import type { FileData } from '~/types/files';

const { files, total, hasMore, loadMore, trash } = defineProps<{
    files: FileData[];
    total?: number;
    hasMore?: boolean;
    loadMore?: () => Promise<unknown>;
    trash?: boolean;
}>();

const fileId = defineModel<string | null>('fileId', { required: true });
const tab = defineModel<string>('tab', { default: 'details' });

const currentUser = useAuthUser();
const publicSettings = usePublicSettings();
const { $toast } = useNuxtApp();
const { t, locale } = useI18n();
const date = useDate();
const { openFile, copyLink, download, move, remove, setSensitive } = useFileActions();
const { restore } = useTrash();

const stage = useTemplateRef('stage');
const strip = useTemplateRef('strip');
const closeButton = useTemplateRef<any>('closeButton');

const { copy } = useClipboard({ legacy: true });

const isZoomed = ref(false);
const isRevealed = ref(false);
const loadedId = ref<string>();
const failedPdfId = ref<string>();
const textContent = ref<string>();
const textMode = ref<'preview' | 'raw'>('preview');
const isSaving = ref(false);
const isMarking = ref(false);
const zoomOrigin = ref('center');
const formErrors = ref();

const fallbackFile = ref<FileData>();

const currentFile = computed(
    () =>
        files.find((f) => f.id === fileId.value) ||
        (fallbackFile.value?.id === fileId.value ? fallbackFile.value : undefined),
);
const file = ref<FileData>();
const isOwner = computed(() => !!file.value && isOwnedItem(file.value));
const isEditable = computed(() => !!file.value && isEditableItem(file.value));
const index = computed(() => files.findIndex((f) => f.id === fileId.value));
const prev = computed(() => files[index.value - 1]);
const next = computed(() => files[index.value + 1]);
const hasNext = computed(() => !!next.value || (index.value > -1 && hasMore));

const stripVirtualizer = useVirtualizer(
    computed(() => ({
        count: files.length,
        horizontal: true,
        gap: 4,
        overscan: 10,
        paddingStart: 16,
        paddingEnd: 16,
        estimateSize: () => 48,
        getKey: (i: number) => files[i]!.id,
        getScrollElement: () => strip.value,
    })),
);

const stripItems = computed(() =>
    stripVirtualizer.value
        .getVirtualItems()
        .map((item) => ({ start: item.start, file: files[item.index]! })),
);

const handleStripWheel = (event: WheelEvent) => {
    if (Math.abs(event.deltaY) <= Math.abs(event.deltaX)) return;

    event.preventDefault();
    strip.value!.scrollLeft += event.deltaY;
};

const goNext = async () => {
    if (!next.value && hasMore) await loadMore?.();

    const target = files[index.value + 1];
    if (target) fileId.value = target.id;
};

const handleOpenFile = () =>
    trash ? window.open(file.value!.directUrl, '_blank') : openFile(file.value!);

const isRestoring = ref(false);
const runningAction = ref<string | null>(null);

const handleRestore = async () => {
    isRestoring.value = true;
    await restore({ files: [file.value!.id] });
    isRestoring.value = false;
};

const handleAction = async (action: { label: string; run: () => unknown }) => {
    runningAction.value = action.label;
    await action.run();
    runningAction.value = null;
};

const handleDeleteForever = () =>
    askConfirmation({
        title: t('fileViewer.deleteForeverTitle', { name: file.value!.name }),
        description: t('fileViewer.deleteForeverDescription'),
        confirmText: t('fileViewer.deleteForever'),
        destructive: true,
        action: async () => {
            try {
                await $fetch('/api/trash/delete', {
                    method: 'POST',
                    body: { files: [file.value!.id] },
                });

                $toast.success(t('fileViewer.deletedForever'));
            } catch (error: any) {
                $toast.error(getErrorMessage(error));
                return false;
            }
        },
    });

const footerActions = computed(() =>
    !file.value
        ? []
        : trash
          ? [
                {
                    label: t('fileViewer.download'),
                    icon: IDownload,
                    run: () => download([file.value!]),
                },
                {
                    label: t('fileViewer.deleteForever'),
                    icon: ITrash2,
                    destructive: true,
                    run: handleDeleteForever,
                },
            ]
          : [
                {
                    label: t('fileViewer.open'),
                    icon: IExternalLink,
                    run: () => openFile(file.value!),
                },
                {
                    label: t('fileViewer.download'),
                    icon: IDownload,
                    run: () => download([file.value!]),
                },
                ...(file.value.folderId && isOwner.value
                    ? [
                          {
                              label: t('fileViewer.removeFromFolder'),
                              icon: IFolderMinus,
                              run: () => move([file.value!], null),
                          },
                      ]
                    : []),
                ...(isEditable.value
                    ? [
                          {
                              label: t('fileViewer.delete'),
                              icon: ITrash2,
                              destructive: true,
                              run: () => remove([file.value!]),
                          },
                      ]
                    : []),
            ],
);

const isImage = computed(() => file.value!.mimeType.startsWith('image/'));
const isVideo = computed(() => isVideoFile(file.value!));
const isAudio = computed(() => file.value!.mimeType.startsWith('audio/'));
const isPdfPreview = computed(
    () => isPdf(file.value!.mimeType) && failedPdfId.value !== file.value!.id,
);
const textLanguage = computed(() => textPreviewLanguage(file.value!.name, file.value!.mimeType));
const isTextPreview = computed(
    () =>
        !!textLanguage.value &&
        !isImage.value &&
        !isVideo.value &&
        !isAudio.value &&
        !isPdfPreview.value,
);
const hasRichPreview = computed(() => textLanguage.value !== 'plaintext');

const handleTextMode = (value: unknown) => value && (textMode.value = value as 'preview' | 'raw');

const handleCopyContent = () => {
    copy(textContent.value!);
    $toast.success(t('view.contentCopied'));
};

const overlayClass = computed(() =>
    cn('transition-opacity', isZoomed.value && 'pointer-events-none opacity-0'),
);

const hasPlaceholder = computed(() => !!file.value!.metadata?.width);
const isLoaded = computed(() => loadedId.value === file.value!.id);

const mediaBoxStyle = computed(() => {
    const { width, height } = file.value!.metadata!;

    return {
        width: `min(100cqw, ${width}px, 100cqh * ${width! / height!})`,
        aspectRatio: `${width} / ${height}`,
    };
});

const isSensitive = computed(
    () =>
        isImage.value &&
        file.value!.piiDetected &&
        currentUser.value!.aiSettings.suppressPii &&
        publicSettings.value.aiEnabled,
);

const isAnalyzing = computed(
    () =>
        !trash &&
        currentUser.value!.aiSettings.enabled &&
        !file.value!.aiAnalyzedAt &&
        !!getAIFileKind(file.value!.fileName),
);

const activeTab = computed({
    get: () => (tab.value === 'insights' && !hasInsights.value ? 'details' : tab.value),
    set: (value) => (tab.value = value),
});

const hasInsights = computed(
    () =>
        publicSettings.value.aiEnabled &&
        (!!file.value!.caption || file.value!.piiDetected || isAnalyzing.value),
);

const details = computed(() => {
    const f = file.value!;

    return [
        { label: t('fileViewer.type'), value: f.mimeType },
        { label: t('fileViewer.size'), value: formatSize(f) },
        ...metadata.value,
        { label: t('fileViewer.folder'), value: f.folder?.name || t('fileViewer.none') },
        {
            label: t('fileViewer.views'),
            value: t('fileViewer.viewsValue', { total: f.views.total, today: f.views.today }),
        },
        { label: t('fileViewer.maxViews'), value: f.maxViews || t('fileViewer.unlimited') },
        {
            label: t('fileViewer.password'),
            value: f.password ? t('fileViewer.protected') : t('fileViewer.none'),
        },
        {
            label: t('fileViewer.expires'),
            value: f.expiresAt ? date(f.expiresAt).format('lll') : t('fileViewer.never'),
        },
        { label: t('fileViewer.uploaded'), value: date(f.createdAt).format('lll') },
    ];
});

const CODECS: Record<string, string> = {
    h264: 'H.264',
    hevc: 'H.265',
    av1: 'AV1',
    vp8: 'VP8',
    vp9: 'VP9',
    aac: 'AAC',
    mp3: 'MP3',
    opus: 'Opus',
    vorbis: 'Vorbis',
    flac: 'FLAC',
    pcm_s16le: 'PCM 16-bit',
    pcm_s24le: 'PCM 24-bit',
};

const CHANNELS: Record<number, string> = {
    1: 'fileViewer.mono',
    2: 'fileViewer.stereo',
    6: 'fileViewer.surround51',
    8: 'fileViewer.surround71',
};

const formatSize = (f: FileData) => filesize(Number(f.size.raw), { locale: locale.value });

const formatNumber = (value: number, digits = 0) =>
    new Intl.NumberFormat(locale.value, { maximumFractionDigits: digits }).format(value);

const formatDuration = (seconds: number) =>
    [Math.floor(seconds / 3600), Math.floor((seconds % 3600) / 60), Math.floor(seconds % 60)]
        .filter((value, i) => value || i)
        .map((value, i) => (i ? String(value).padStart(2, '0') : value))
        .join(':');

const formatBitrate = (bps: number) =>
    bps >= 1e6 ? `${formatNumber(bps / 1e6, 1)} Mbps` : `${formatNumber(bps / 1e3)} kbps`;

const metadata = computed(() => {
    const m = file.value!.metadata;
    if (!m) return [];

    return [
        { label: t('fileViewer.resolution'), value: m.width && `${m.width} × ${m.height}` },
        { label: t('fileViewer.duration'), value: m.duration && formatDuration(m.duration) },
        { label: t('fileViewer.frameRate'), value: m.fps && `${formatNumber(m.fps, 2)} fps` },
        { label: t('fileViewer.bitrate'), value: m.bitrate && formatBitrate(m.bitrate) },
        {
            label: t('fileViewer.codec'),
            value: [m.videoCodec, m.audioCodec]
                .filter(Boolean)
                .map((codec) => CODECS[codec!] ?? codec!.toUpperCase())
                .join(' · '),
        },
        {
            label: t('fileViewer.audio'),
            value:
                m.sampleRate &&
                `${formatNumber(m.sampleRate / 1000, 1)} kHz · ${CHANNELS[m.channels!] ? t(CHANNELS[m.channels!]!) : t('fileViewer.channelCount', m.channels!)}`,
        },
        { label: t('fileViewer.format'), value: m.format },
        {
            label: t('fileViewer.color'),
            value:
                m.colorSpace &&
                [
                    m.colorSpace,
                    m.bitDepth && t('fileViewer.bitDepth', { n: m.bitDepth }),
                    m.hasAlpha && t('fileViewer.alpha'),
                ]
                    .filter(Boolean)
                    .join(' · '),
        },
        { label: t('fileViewer.frames'), value: m.frames },
        { label: t('fileViewer.pages'), value: m.pages && formatNumber(m.pages) },
    ].filter((row) => row.value);
});

const editData = reactive({
    name: '',
    fileName: '',
    folderId: null as string | null,
    expiration: 'labels.expiration.never',
    maxViews: 0,
    password: '',
});

const resetEditData = () => {
    const f = currentFile.value!;

    Object.assign(editData, {
        name: f.name,
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
            body: isOwner.value
                ? {
                      name: editData.name,
                      fileName: `${editData.fileName}${getExtname(f.fileName)}`,
                      folderId: editData.folderId,
                      maxViews: editData.maxViews,
                      password: editData.password || null,
                      ...(editData.expiration !== expiration && {
                          expiration: EXPIRATIONS.find((e) => e.label === editData.expiration)!
                              .value,
                      }),
                  }
                : { name: editData.name },
        });

        $toast.success(t('fileViewer.fileUpdated'));
        tab.value = 'details';
    } catch (error: any) {
        formErrors.value = error.data?.data?.formErrors;
        if (!formErrors.value) $toast.error(getErrorMessage(error));
    }

    isSaving.value = false;
};

const handleNotSensitive = async () => {
    isMarking.value = true;
    await setSensitive(file.value!, false);
    isMarking.value = false;
};

const setZoomOrigin = (event: MouseEvent) => {
    const target = event.currentTarget as HTMLElement;
    const parent = target.parentElement!.getBoundingClientRect();
    const left = parent.left + (parent.width - target.offsetWidth) / 2;
    const top = parent.top + (parent.height - target.offsetHeight) / 2;
    const x = Math.min(Math.max(((event.clientX - left) / target.offsetWidth) * 100, 0), 100);
    const y = Math.min(Math.max(((event.clientY - top) / target.offsetHeight) * 100, 0), 100);

    zoomOrigin.value = `${x}% ${y}%`;
};

const handleZoom = (event: MouseEvent) => {
    if (isSensitive.value && !isRevealed.value) return;
    if (!isZoomed.value) setZoomOrigin(event);

    isZoomed.value = !isZoomed.value;
};

const handleBackdropClick = () => {
    if (isZoomed.value) isZoomed.value = false;
    else fileId.value = null;
};

watch(
    fileId,
    async (value) => {
        isZoomed.value = false;
        isRevealed.value = false;

        if (value && currentFile.value) resetEditData();

        if (value && !currentFile.value && !trash) {
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

watch(
    [index, strip],
    async ([value, el], [, oldEl]) => {
        if (!el || value < 0) return;

        await nextTick();
        stripVirtualizer.value.scrollToIndex(value, {
            align: 'center',
            behavior: oldEl ? 'smooth' : 'auto',
        });
    },
    { flush: 'post' },
);

watch(
    () => stripVirtualizer.value.getVirtualItems().at(-1)?.index,
    (last) => {
        if (last !== undefined && hasMore && last >= files.length - 5) loadMore?.();
    },
);

useSwipe(stage, {
    onSwipeEnd: (_, direction) => {
        if (isZoomed.value || isTextPreview.value) return;
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
    resource: trash ? 'trash' : 'files',
    refresh: () => null,
    map: (fn) => fallbackFile.value && (fallbackFile.value = fn(fallbackFile.value)),
    remove: (ids) =>
        fallbackFile.value &&
        ids.includes(fallbackFile.value.id) &&
        (fallbackFile.value = undefined),
});
</script>
