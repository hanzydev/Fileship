<template>
    <UiCard>
        <form class="contents" @submit.prevent="handleSaveSchedule">
            <UiCardHeader>
                <UiCardTitle>{{ $t('backups.schedule.title') }}</UiCardTitle>
                <UiCardDescription>{{ $t('backups.schedule.description') }}</UiCardDescription>
            </UiCardHeader>
            <UiCardContent class="flex flex-col gap-6">
                <UiAlert v-if="!publicSettings.scheduledBackupsEnabled">
                    <ICircleAlert />
                    <UiAlertTitle>{{ $t('backups.schedule.disabled') }}</UiAlertTitle>
                    <UiAlertDescription>
                        {{ $t('backups.schedule.disabledDescription') }}
                    </UiAlertDescription>
                </UiAlert>

                <div v-if="!schedule" class="grid gap-4 sm:grid-cols-2">
                    <UiSkeleton v-for="i in 2" :key="i" class="h-16" />
                </div>

                <template v-else>
                    <UiFieldGroup class="grid gap-x-4 sm:grid-cols-2">
                        <UiField>
                            <UiFieldLabel for="backup-frequency">
                                {{ $t('backups.schedule.frequency') }}
                            </UiFieldLabel>
                            <UiSelect
                                v-model="scheduleForm.frequency"
                                :disabled="!publicSettings.scheduledBackupsEnabled"
                            >
                                <UiSelectTrigger id="backup-frequency" class="w-full">
                                    <UiSelectValue />
                                </UiSelectTrigger>
                                <UiSelectContent>
                                    <UiSelectItem
                                        v-for="frequency in FREQUENCIES"
                                        :key="frequency"
                                        :value="frequency"
                                    >
                                        {{ $t(`backups.schedule.frequencies.${frequency}`) }}
                                    </UiSelectItem>
                                </UiSelectContent>
                            </UiSelect>
                        </UiField>

                        <UiField v-if="scheduleForm.frequency === BackupFrequency.Weekly">
                            <UiFieldLabel for="backup-weekday">
                                {{ $t('backups.schedule.weekday') }}
                            </UiFieldLabel>
                            <UiSelect
                                v-model="scheduleForm.weekday"
                                :disabled="!publicSettings.scheduledBackupsEnabled"
                            >
                                <UiSelectTrigger id="backup-weekday" class="w-full">
                                    <UiSelectValue />
                                </UiSelectTrigger>
                                <UiSelectContent>
                                    <UiSelectItem
                                        v-for="weekday in WEEKDAYS"
                                        :key="weekday"
                                        :value="weekday"
                                    >
                                        {{ date().day(weekday).format('dddd') }}
                                    </UiSelectItem>
                                </UiSelectContent>
                            </UiSelect>
                        </UiField>

                        <UiField v-if="scheduleForm.frequency !== BackupFrequency.Off">
                            <UiFieldLabel for="backup-hour">
                                {{ $t('backups.schedule.time') }}
                            </UiFieldLabel>
                            <UiSelect
                                v-model="scheduleForm.hour"
                                :disabled="!publicSettings.scheduledBackupsEnabled"
                            >
                                <UiSelectTrigger id="backup-hour" class="w-full">
                                    <UiSelectValue />
                                </UiSelectTrigger>
                                <UiSelectContent>
                                    <UiSelectItem v-for="hour in 24" :key="hour" :value="hour - 1">
                                        {{
                                            date()
                                                .hour(hour - 1)
                                                .minute(0)
                                                .format('LT')
                                        }}
                                    </UiSelectItem>
                                </UiSelectContent>
                            </UiSelect>
                            <UiFieldDescription v-if="timezone">
                                {{ $t('backups.schedule.timezone', { timezone }) }}
                            </UiFieldDescription>
                        </UiField>

                        <UiField v-if="scheduleForm.frequency !== BackupFrequency.Off">
                            <UiFieldLabel>{{ $t('backups.schedule.keep') }}</UiFieldLabel>
                            <UiNumberField
                                v-model="scheduleForm.keep"
                                :min="1"
                                :max="maxKeep"
                                :disabled="!publicSettings.scheduledBackupsEnabled"
                            >
                                <UiNumberFieldContent>
                                    <UiNumberFieldDecrement />
                                    <UiNumberFieldInput />
                                    <UiNumberFieldIncrement />
                                </UiNumberFieldContent>
                            </UiNumberField>
                            <UiFieldDescription>
                                {{
                                    backupLimit === null
                                        ? $t('backups.schedule.keepDescription')
                                        : $t('backups.schedule.keepLimit', { limit: backupLimit })
                                }}
                            </UiFieldDescription>
                        </UiField>
                    </UiFieldGroup>

                    <UiItemGroup
                        v-if="publicSettings.scheduledBackupsEnabled"
                        class="grid gap-2 sm:grid-cols-2"
                    >
                        <UiItem variant="muted" size="sm">
                            <UiItemMedia variant="icon">
                                <ICalendarClock />
                            </UiItemMedia>
                            <UiItemContent>
                                <UiItemTitle>{{ $t('backups.schedule.nextRun') }}</UiItemTitle>
                                <UiItemDescription>
                                    {{
                                        schedule.nextRunAt
                                            ? date(schedule.nextRunAt).format('lll')
                                            : $t('backups.schedule.notScheduled')
                                    }}
                                </UiItemDescription>
                            </UiItemContent>
                        </UiItem>
                        <UiItem variant="muted" size="sm">
                            <UiItemMedia variant="icon">
                                <IHistory />
                            </UiItemMedia>
                            <UiItemContent>
                                <UiItemTitle>{{ $t('backups.schedule.lastRun') }}</UiItemTitle>
                                <UiItemDescription
                                    :class="
                                        schedule.lastRunStatus &&
                                        schedule.lastRunStatus !== BackupRunStatus.Created &&
                                        'text-destructive'
                                    "
                                >
                                    {{
                                        schedule.lastRunAt
                                            ? $t(
                                                  `backups.schedule.status.${schedule.lastRunStatus}`,
                                                  {
                                                      date: date(schedule.lastRunAt).format('lll'),
                                                  },
                                              )
                                            : $t('backups.schedule.never')
                                    }}
                                </UiItemDescription>
                            </UiItemContent>
                        </UiItem>
                    </UiItemGroup>
                </template>
            </UiCardContent>
            <UiCardFooter class="justify-end">
                <UiButton
                    type="submit"
                    :disabled="
                        isSavingSchedule ||
                        !isScheduleDirty ||
                        !publicSettings.scheduledBackupsEnabled
                    "
                >
                    <UiSpinner v-if="isSavingSchedule" />
                    {{ $t('backups.schedule.save') }}
                </UiButton>
            </UiCardFooter>
        </form>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('backups.title') }}</UiCardTitle>
            <UiCardDescription>
                {{ $t('backups.description') }}
            </UiCardDescription>
            <UiCardAction class="flex gap-2">
                <UiButton
                    variant="outline"
                    :disabled="uploadProgress !== null"
                    @click="backupInput!.click()"
                >
                    <IUpload />
                    <span class="hidden sm:inline">{{ $t('backups.upload') }}</span>
                </UiButton>
                <UiButton :disabled="isCreating" @click="handleCreate">
                    <UiSpinner v-if="isCreating" />
                    <IPlus v-else />
                    <span class="hidden sm:inline">{{ $t('backups.create') }}</span>
                </UiButton>
            </UiCardAction>
        </UiCardHeader>
        <UiCardContent class="flex flex-col gap-3">
            <input
                ref="backupInput"
                type="file"
                accept=".tgz"
                class="sr-only"
                @change="handleUpload"
            />

            <UiCard v-if="uploadProgress !== null" size="sm">
                <UiCardContent class="flex flex-col gap-2">
                    <div class="flex items-center justify-between">
                        <TextBlock variant="small">{{ $t('backups.uploading') }}</TextBlock>
                        <TextBlock variant="muted">
                            {{ $t('backups.progress', { progress: uploadProgress }) }}
                        </TextBlock>
                    </div>
                    <UiProgress :model-value="uploadProgress" />
                </UiCardContent>
            </UiCard>

            <div v-if="isLoading" class="flex flex-col gap-2">
                <ListCardSkeleton v-for="i in 3" :key="i" media="size-5" />
            </div>

            <EmptyState
                v-else-if="isError"
                :icon="ICircleAlert"
                :title="$t('backups.loadFailed')"
                :description="$t('backups.loadFailedDescription')"
            >
                <UiButton variant="outline" @click="refresh">{{ $t('backups.tryAgain') }}</UiButton>
            </EmptyState>

            <EmptyState
                v-else-if="!backups.length"
                :icon="IArchive"
                :title="$t('backups.empty')"
                :description="$t('backups.emptyDescription')"
            />

            <VirtualList
                v-else
                :items="backups"
                :item-key="(b) => b.id"
                :estimate-size="66"
                :loading="isLoadingMore"
                @reach-end="loadMore"
            >
                <template #skeleton>
                    <ListCardSkeleton media="size-5" />
                </template>
                <template #default="{ item }">
                    <UiCard size="sm">
                        <UiCardContent class="flex min-h-8 items-center gap-3">
                            <IArchive class="size-5 shrink-0 text-muted-foreground" />
                            <div class="flex min-w-0 flex-1 flex-col">
                                <div class="flex items-center gap-2">
                                    <TextBlock variant="small">
                                        {{ date(item.createdAt).format('lll') }}
                                    </TextBlock>
                                    <UiBadge v-if="item.automatic" variant="secondary">
                                        {{ $t('backups.automatic') }}
                                    </UiBadge>
                                </div>
                                <TextBlock variant="muted" class="text-xs">
                                    {{ filesize(Number(item.size.raw), { locale }) }}
                                </TextBlock>
                            </div>
                            <UiButton variant="outline" size="sm" @click="handleRestore(item)">
                                <IHistory />
                                {{ $t('backups.restore') }}
                            </UiButton>
                            <UiDropdownMenu>
                                <UiDropdownMenuTrigger as-child>
                                    <UiButton variant="ghost" size="icon-sm">
                                        <IEllipsis />
                                    </UiButton>
                                </UiDropdownMenuTrigger>
                                <UiDropdownMenuContent align="end" class="w-56">
                                    <UiDropdownMenuItem
                                        @select="downloadFile(`/api/users/@me/backups/${item.id}`)"
                                    >
                                        <IDownload />
                                        {{ $t('backups.download') }}
                                    </UiDropdownMenuItem>
                                    <UiDropdownMenuItem
                                        variant="destructive"
                                        @select="handleDelete(item)"
                                    >
                                        <ITrash2 />
                                        {{ $t('backups.delete') }}
                                    </UiDropdownMenuItem>
                                </UiDropdownMenuContent>
                            </UiDropdownMenu>
                        </UiCardContent>
                    </UiCard>
                </template>
            </VirtualList>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
import { filesize } from 'filesize';
import { nanoid } from 'nanoid';

import { IArchive, ICircleAlert } from '#components';
import { BackupFrequency, BackupRunStatus } from '#shared/prisma/enums';

import type { BackupData, BackupScheduleData } from '~/types/backups';

const FREQUENCIES = [BackupFrequency.Off, BackupFrequency.Daily, BackupFrequency.Weekly];
const WEEKDAYS = [1, 2, 3, 4, 5, 6, 0];

const publicSettings = usePublicSettings();
const schedule = useBackupSchedule();
const { $toast } = useNuxtApp();
const { t, locale } = useI18n();
const date = useDate();

const backupInput = useTemplateRef('backupInput');

const {
    items: backups,
    isLoading,
    isError,
    isLoadingMore,
    loadMore,
    refresh,
} = usePaginatedList<BackupData>('backups', '/api/users/@me/backups');

const isCreating = ref(false);
const uploadProgress = ref<number | null>(null);

const timezone = ref<string>();
const isSavingSchedule = ref(false);
const scheduleForm = reactive({
    frequency: BackupFrequency.Off as BackupFrequency,
    hour: 3,
    weekday: 1,
    keep: 7,
});

const backupLimit = computed(() => schedule.value?.backupLimit ?? null);
const maxKeep = computed(() => backupLimit.value ?? 100);

const pickSchedule = ({ frequency, hour, weekday, keep }: BackupScheduleData) => ({
    frequency,
    hour,
    weekday,
    keep,
});

const isScheduleDirty = computed(
    () =>
        !!schedule.value &&
        (JSON.stringify(scheduleForm) !== JSON.stringify(pickSchedule(schedule.value)) ||
            (schedule.value.timezone ?? timezone.value) !== timezone.value),
);

watch(
    schedule,
    (value) => {
        if (value) Object.assign(scheduleForm, pickSchedule(value));
    },
    { immediate: true },
);

onMounted(async () => {
    timezone.value = Intl.DateTimeFormat().resolvedOptions().timeZone;

    try {
        schedule.value = await $fetch('/api/users/@me/backups/schedule');
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }
});

const handleSaveSchedule = async () => {
    isSavingSchedule.value = true;

    try {
        schedule.value = await $fetch('/api/users/@me/backups/schedule', {
            method: 'PUT',
            body: { ...scheduleForm, timezone: timezone.value },
        });
        $toast.success(t('backups.schedule.saved'));
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    isSavingSchedule.value = false;
};

const handleCreate = async () => {
    isCreating.value = true;

    try {
        await $fetch('/api/users/@me/backups', { method: 'POST' });
        $toast.success(t('backups.creating'));
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    isCreating.value = false;
};

const handleUpload = async (event: Event) => {
    const input = event.target as HTMLInputElement;
    const file = input.files![0]!;
    input.value = '';

    if (!file.name.endsWith('.tgz')) {
        $toast.error(t('backups.invalidFile'));
        return;
    }

    const chunkSize = publicSettings.value.fileChunkSize * 1024 * 1024;
    const chunks = Math.ceil(file.size / chunkSize);
    const uploadId = nanoid();

    uploadProgress.value = 0;

    try {
        for (let i = 0; i < chunks; i++) {
            const start = i * chunkSize;
            const end = Math.min(file.size, start + chunkSize);

            const formData = new FormData();
            formData.append(
                'backup',
                new Blob([file.slice(start, end)], { type: file.type }),
                file.name,
            );
            formData.append('currentChunk', (i + 1).toString());
            formData.append('totalChunks', chunks.toString());
            formData.append('chunkOffset', start.toString());
            formData.append('uploadId', uploadId);

            await $fetch('/api/users/@me/backups/upload', {
                method: 'POST',
                body: formData,
            });

            uploadProgress.value = Math.round((end / file.size) * 100);
        }

        $toast.success(t('backups.uploaded'));
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    uploadProgress.value = null;
};

const handleRestore = async (backup: BackupData) => {
    await askConfirmation({
        title: t('backups.restoreTitle'),
        description: t('backups.restoreDescription'),
        confirmText: t('backups.restore'),
        destructive: true,
        action: async () => {
            try {
                const result = await withVerification((verificationData) =>
                    $fetch(`/api/users/@me/backups/${backup.id}/load`, {
                        method: 'POST',
                        body: { verificationData },
                    }),
                );

                if (result) $toast.success(t('backups.restoring'));
            } catch (error: any) {
                $toast.error(getErrorMessage(error));
            }
        },
    });
};

const handleDelete = async (backup: BackupData) => {
    await askConfirmation({
        title: t('backups.deleteTitle'),
        description: t('backups.deleteDescription'),
        confirmText: t('backups.delete'),
        destructive: true,
        action: async () => {
            try {
                await $fetch(`/api/users/@me/backups/${backup.id}`, { method: 'DELETE' });
                $toast.success(t('backups.deleted'));
            } catch (error: any) {
                $toast.error(getErrorMessage(error));
            }
        },
    });
};
</script>
