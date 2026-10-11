<template>
    <Transition
        enter-active-class="animate-in fade-in slide-in-from-bottom-4 duration-200"
        leave-active-class="animate-out fade-out slide-out-to-bottom-4 duration-200"
    >
        <UiCard
            v-if="uploadingFiles.length"
            class="fixed right-4 bottom-4 z-40 w-[calc(100%-2rem)] gap-0 py-0 sm:w-96"
        >
            <UiCollapsible v-model:open="isExpanded">
                <div class="flex items-center gap-3 px-4 py-3">
                    <UiSpinner v-if="activeCount" />
                    <ICircleAlert v-else class="size-4 text-destructive" />
                    <TextBlock variant="small" class="flex-1">{{ title }}</TextBlock>
                    <UiCollapsibleTrigger as-child>
                        <UiButton variant="ghost" size="icon-sm">
                            <IChevronDown
                                :class="cn('transition-transform', !isExpanded && 'rotate-180')"
                            />
                        </UiButton>
                    </UiCollapsibleTrigger>
                </div>
                <UiCollapsibleContent>
                    <UiSeparator />
                    <div class="flex max-h-72 flex-col gap-3 overflow-y-auto p-4">
                        <div
                            v-for="(file, index) in uploadingFiles"
                            :key="`${file.name}-${index}`"
                            class="flex flex-col gap-1.5"
                        >
                            <div class="flex items-center gap-2">
                                <TextBlock variant="small" class="flex-1 truncate">
                                    {{ file.name }}
                                </TextBlock>
                                <template v-if="file.status!.error">
                                    <UiButton variant="ghost" size="icon-xs" @click="retry(file)">
                                        <IRotateCw />
                                    </UiButton>
                                    <UiButton variant="ghost" size="icon-xs" @click="dismiss(file)">
                                        <IX />
                                    </UiButton>
                                </template>
                                <ICheck
                                    v-else-if="file.status!.completed"
                                    class="size-4 text-primary"
                                />
                                <TextBlock v-else variant="muted">
                                    {{ file.status!.progress.percent }}%
                                </TextBlock>
                            </div>
                            <UiProgress
                                v-if="!file.status!.error"
                                :model-value="file.status!.progress.percent"
                            />
                            <TextBlock variant="muted" class="truncate">
                                <span v-if="file.status!.error" class="text-destructive">
                                    {{ file.status!.error }}
                                </span>
                                <template v-else-if="!file.status!.started">Waiting...</template>
                                <template v-else-if="!file.status!.completed">
                                    {{ filesize(file.status!.progress.speed) }}/s ·
                                    {{ formatEta(file.status!.progress.eta) }} left
                                </template>
                                <template v-else>Uploaded</template>
                            </TextBlock>
                        </div>
                    </div>
                </UiCollapsibleContent>
            </UiCollapsible>
        </UiCard>
    </Transition>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';
import { filesize } from 'filesize';

const uploadingFiles = useUploadingFiles();
const { retry, dismiss } = useUploader();

const isExpanded = ref(true);

const activeCount = computed(() => uploadingFiles.value.filter((f) => !f.status!.error).length);
const failedCount = computed(() => uploadingFiles.value.length - activeCount.value);

const title = computed(() =>
    activeCount.value
        ? `Uploading ${activeCount.value} file${activeCount.value > 1 ? 's' : ''}`
        : `${failedCount.value} upload${failedCount.value > 1 ? 's' : ''} failed`,
);

const formatEta = (seconds: number) => dayjs.duration(seconds, 'seconds').humanize();
</script>
