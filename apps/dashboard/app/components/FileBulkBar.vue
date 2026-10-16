<template>
    <Transition
        enter-active-class="animate-in fade-in slide-in-from-bottom-4 duration-150"
        leave-active-class="animate-out fade-out slide-out-to-bottom-4 duration-150"
    >
        <UiCard
            v-if="selected.length"
            size="sm"
            :class="
                cn(
                    'fixed bottom-4 left-1/2 z-30 -translate-x-1/2 flex-row items-center gap-3 py-2 pr-2 pl-4 shadow-lg',
                    uploadingFiles.length && 'bottom-24 sm:bottom-4',
                )
            "
        >
            <TextBlock variant="small" class="whitespace-nowrap">
                {{ $t('files.selectedCount', { count: selected.length }) }}
            </TextBlock>
            <UiButtonGroup>
                <UiButton
                    variant="outline"
                    size="sm"
                    :aria-label="isAllSelected ? $t('files.deselectAll') : $t('files.selectAll')"
                    :disabled="!!busyAction"
                    @click="handleSelectAll"
                >
                    <UiSpinner v-if="busyAction === 'select'" />
                    <IListX v-else-if="isAllSelected" />
                    <IListChecks v-else />
                    <span class="hidden sm:inline">
                        {{ isAllSelected ? $t('files.deselectAll') : $t('files.selectAll') }}
                    </span>
                </UiButton>
                <UiButton
                    variant="outline"
                    size="sm"
                    :aria-label="$t('files.move')"
                    :disabled="!!busyAction"
                    @click="handleMove"
                >
                    <IFolderInput />
                    <span class="hidden sm:inline">{{ $t('files.move') }}</span>
                </UiButton>
                <UiButton
                    variant="outline"
                    size="sm"
                    :aria-label="$t('files.download')"
                    :disabled="!!busyAction"
                    @click="handleDownload"
                >
                    <UiSpinner v-if="busyAction === 'download'" />
                    <IDownload v-else />
                    <span class="hidden sm:inline">{{ $t('files.download') }}</span>
                </UiButton>
                <UiButton
                    variant="outline"
                    size="sm"
                    class="text-destructive hover:text-destructive"
                    :aria-label="$t('files.delete')"
                    :disabled="!!busyAction"
                    @click="handleDelete"
                >
                    <ITrash2 />
                    <span class="hidden sm:inline">{{ $t('files.delete') }}</span>
                </UiButton>
            </UiButtonGroup>
            <UiButton
                variant="ghost"
                size="icon-sm"
                :aria-label="$t('files.clearSelection')"
                @click="selected = []"
            >
                <IX />
            </UiButton>
        </UiCard>
    </Transition>

    <FileMoveDialog v-model="moveTargets" @moved="selected = []" />
</template>

<script setup lang="ts">
import type { FileData } from '~/types/files';

const { items, total, query } = defineProps<{
    items: FileData[];
    total: number;
    query?: Record<string, unknown>;
}>();

const selected = defineModel<string[]>({ required: true });

const uploadingFiles = useUploadingFiles();

const { t } = useI18n();
const { $toast } = useNuxtApp();
const { download, remove } = useFileActions();

const busyAction = ref<'select' | 'download' | null>(null);
const moveTargets = ref<Pick<FileData, 'id' | 'folderId'>[]>([]);

const isAllSelected = computed(() => selected.value.length >= total);

const handleSelectAll = async () => {
    if (isAllSelected.value) {
        selected.value = [];
        return;
    }

    if (!query) {
        selected.value = items.map((file) => file.id);
        return;
    }

    busyAction.value = 'select';

    try {
        selected.value = await $fetch('/api/files/ids', { query });
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    busyAction.value = null;
};

const handleMove = () => {
    const folderIds = new Set(
        selected.value.map((id) => items.find((file) => file.id === id)?.folderId),
    );
    const folderId = folderIds.size === 1 ? [...folderIds][0] : undefined;

    moveTargets.value = selected.value.map((id) => ({ id, folderId: folderId ?? null }));
};

const handleDownload = async () => {
    const loaded = items.filter((file) => selected.value.includes(file.id));
    const missing = selected.value.filter((id) => !loaded.some((file) => file.id === id));

    busyAction.value = 'download';

    try {
        const chunks = Array.from({ length: Math.ceil(missing.length / 100) }, (_, index) =>
            missing.slice(index * 100, index * 100 + 100),
        );

        const pages = await Promise.all(
            chunks.map((ids) =>
                $fetch('/api/files', { query: { ids: ids.join(','), limit: ids.length } }),
            ),
        );

        download([...loaded, ...(pages.flatMap((page) => page.items) as never[])]);
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    busyAction.value = null;
};

const handleDelete = async () => {
    const files = selected.value.map(
        (id) => items.find((file) => file.id === id) ?? { id, name: t('files.fallbackName') },
    );

    if (await remove(files)) selected.value = [];
};
</script>
