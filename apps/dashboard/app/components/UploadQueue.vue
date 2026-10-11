<template>
    <Transition
        enter-active-class="animate-in fade-in slide-in-from-bottom-4 duration-200"
        leave-active-class="animate-out fade-out slide-out-to-bottom-4 duration-200"
    >
        <UiCard
            v-if="uploadingFiles.length"
            size="sm"
            class="fixed right-4 bottom-4 z-40 w-[calc(100%-2rem)] sm:w-96"
        >
            <UiCollapsible v-model:open="isExpanded" class="flex flex-col gap-4">
                <UiCardHeader>
                    <UiCardTitle class="flex items-center gap-2">
                        <UiSpinner v-if="activeCount" />
                        <ICircleAlert v-else class="size-4 text-destructive" />
                        {{ title }}
                    </UiCardTitle>
                    <UiCardAction>
                        <UiCollapsibleTrigger as-child>
                            <UiButton
                                variant="ghost"
                                size="icon-sm"
                                :aria-label="isExpanded ? 'Collapse' : 'Expand'"
                            >
                                <IChevronDown
                                    :class="cn('transition-transform', !isExpanded && 'rotate-180')"
                                />
                            </UiButton>
                        </UiCollapsibleTrigger>
                    </UiCardAction>
                </UiCardHeader>
                <UiCollapsibleContent>
                    <UiCardContent class="flex max-h-72 flex-col gap-3 overflow-y-auto">
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
                            <UiFieldError
                                v-if="file.status!.error"
                                :errors="[file.status!.error]"
                            />
                            <TextBlock v-else variant="muted" class="truncate text-xs">
                                <template v-if="!file.status!.started">Waiting...</template>
                                <template v-else-if="!file.status!.completed">
                                    {{ filesize(file.status!.progress.speed) }}/s ·
                                    {{ formatEta(file.status!.progress.eta) }} left
                                </template>
                                <template v-else>Uploaded</template>
                            </TextBlock>
                        </div>
                    </UiCardContent>
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
