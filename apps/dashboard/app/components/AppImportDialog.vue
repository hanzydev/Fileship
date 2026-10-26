<template>
    <UiDialog v-model:open="open">
        <UiDialogContent class="sm:max-w-xl">
            <UiDialogHeader>
                <UiDialogTitle>{{ $t('instance.appImport.title') }}</UiDialogTitle>
                <UiDialogDescription>
                    {{ $t('instance.appImport.description') }}
                </UiDialogDescription>
            </UiDialogHeader>

            <div class="-mx-6 flex max-h-[60vh] flex-col gap-4 overflow-y-auto px-6">
                <ol class="flex list-decimal flex-col gap-3 pl-5 text-sm">
                    <li class="space-y-2">
                        <i18n-t
                            v-if="storage?.id"
                            keypath="instance.appImport.stepStorage"
                            tag="span"
                            scope="global"
                        >
                            <template #name>
                                <strong>{{ storage.name }}</strong>
                            </template>
                        </i18n-t>
                        <template v-else>
                            <span>{{ $t('instance.appImport.stepFolder') }}</span>
                            <UiInputGroup v-if="importPath">
                                <UiInputGroupInput
                                    :model-value="importPath"
                                    :aria-label="$t('instance.import.folderLabel')"
                                    readonly
                                />
                                <UiInputGroupAddon align="inline-end">
                                    <UiInputGroupButton
                                        size="icon-xs"
                                        type="button"
                                        :aria-label="$t('instance.import.copyPath')"
                                        @click="handleCopyPath"
                                    >
                                        <ICopy />
                                    </UiInputGroupButton>
                                </UiInputGroupAddon>
                            </UiInputGroup>
                        </template>
                    </li>
                    <li class="space-y-2">
                        <span>{{ $t('instance.appImport.stepSource') }}</span>
                        <ul class="flex list-disc flex-col gap-1 pl-5 text-muted-foreground">
                            <li>{{ $t('instance.appImport.sourceZipline') }}</li>
                            <i18n-t
                                keypath="instance.appImport.sourceXBackBone"
                                tag="li"
                                scope="global"
                            >
                                <template #path>
                                    <code>{{ XBACKBONE_DATABASE }}</code>
                                </template>
                            </i18n-t>
                            <i18n-t
                                keypath="instance.appImport.sourceChibisafe"
                                tag="li"
                                scope="global"
                            >
                                <template #path>
                                    <code>{{ CHIBISAFE_DATABASE }}</code>
                                </template>
                            </i18n-t>
                        </ul>
                        <input
                            ref="sourceInput"
                            type="file"
                            accept=".json,.db,.sqlite,.sqlite3"
                            class="sr-only"
                            @change="handleChooseSource"
                        />
                        <UiButton
                            variant="outline"
                            :disabled="isReading"
                            @click="sourceInput?.click()"
                        >
                            <UiSpinner v-if="isReading" />
                            <IFileUp v-else />
                            {{
                                preview
                                    ? $t('instance.appImport.chooseOther')
                                    : $t('instance.appImport.chooseFile')
                            }}
                        </UiButton>
                    </li>
                </ol>

                <EmptyState
                    v-if="error"
                    :icon="ICircleAlert"
                    :title="$t('instance.appImport.readFailed')"
                    :description="error"
                >
                    <UiButton v-if="sourceId" variant="outline" @click="loadPreview">
                        {{ $t('instance.import.tryAgain') }}
                    </UiButton>
                </EmptyState>

                <template v-else-if="preview">
                    <UiAlert>
                        <IDatabaseZap />
                        <UiAlertTitle>
                            {{
                                preview.version
                                    ? $t('instance.appImport.detectedVersion', {
                                          app: IMPORT_APPS[preview.app],
                                          version: preview.version,
                                      })
                                    : $t('instance.appImport.detected', {
                                          app: IMPORT_APPS[preview.app],
                                      })
                            }}
                        </UiAlertTitle>
                        <UiAlertDescription>
                            {{
                                $t(
                                    'instance.appImport.found',
                                    {
                                        found: preview.found,
                                        n: preview.files,
                                        size: filesize(preview.size, { locale }),
                                    },
                                    preview.files,
                                )
                            }}
                            <template v-if="extras.length">
                                {{
                                    $t('instance.appImport.contents', {
                                        items: formatList(extras, 'conjunction'),
                                    })
                                }}
                            </template>
                        </UiAlertDescription>
                    </UiAlert>

                    <UiAlert v-if="preview.found < preview.files">
                        <ITriangleAlert />
                        <UiAlertTitle>
                            {{
                                $t(
                                    'instance.appImport.missingTitle',
                                    { n: preview.files - preview.found },
                                    preview.files - preview.found,
                                )
                            }}
                        </UiAlertTitle>
                        <UiAlertDescription class="flex flex-col items-start gap-2">
                            {{ $t('instance.appImport.missingDescription') }}
                            <UiButton
                                variant="outline"
                                size="sm"
                                :disabled="isReading"
                                @click="loadPreview"
                            >
                                {{ $t('instance.import.checkAgain') }}
                            </UiButton>
                        </UiAlertDescription>
                    </UiAlert>

                    <div class="flex flex-col gap-2">
                        <TextBlock variant="small">
                            {{ $t('instance.appImport.accounts') }}
                        </TextBlock>
                        <TextBlock variant="muted" class="text-sm">
                            {{ $t('instance.appImport.accountsDescription') }}
                        </TextBlock>
                        <UiItemGroup class="gap-2">
                            <UiItem
                                v-for="user in preview.users"
                                :key="user.id"
                                variant="muted"
                                size="sm"
                            >
                                <UiItemContent class="min-w-0">
                                    <UiItemTitle class="truncate">{{ user.username }}</UiItemTitle>
                                    <UiItemDescription>
                                        {{ userContents(user) }}
                                    </UiItemDescription>
                                </UiItemContent>
                                <UiItemActions class="w-40 sm:w-48">
                                    <UserPicker
                                        :label="targetLabel(user)"
                                        :selected="targetId(user)"
                                        @select="targets[user.id] = $event"
                                    >
                                        <template #icon>
                                            <component :is="targetIcon(user)" />
                                        </template>
                                        <template #default="{ close }">
                                            <UiCommandItem
                                                value="new"
                                                :data-checked="targets[user.id] === 'new'"
                                                @select="handleTarget(user, 'new', close)"
                                            >
                                                <IUserPlus />
                                                {{ $t('instance.appImport.newAccount') }}
                                            </UiCommandItem>
                                            <UiCommandItem
                                                value="skip"
                                                :data-checked="targets[user.id] === null"
                                                @select="handleTarget(user, null, close)"
                                            >
                                                <IUserX />
                                                {{ $t('instance.appImport.skipAccount') }}
                                            </UiCommandItem>
                                        </template>
                                    </UserPicker>
                                </UiItemActions>
                            </UiItem>
                        </UiItemGroup>
                    </div>

                    <ul class="flex list-disc flex-col gap-1 pl-5 text-sm text-muted-foreground">
                        <i18n-t keypath="instance.appImport.ruleMoved" tag="li" scope="global">
                            <template #folder>
                                <code>{{ UPLOADS_FOLDER }}</code>
                            </template>
                        </i18n-t>
                        <li>{{ $t('instance.appImport.rulePermissions') }}</li>
                        <li>{{ $t('instance.appImport.ruleLocked') }}</li>
                        <li>{{ $t('instance.appImport.ruleSkipped') }}</li>
                        <li>{{ $t('instance.appImport.ruleQuota') }}</li>
                    </ul>
                </template>
            </div>

            <UiDialogFooter>
                <UiButton variant="outline" @click="open = false">
                    {{ $t('common.cancel') }}
                </UiButton>
                <UiButton :disabled="!canStart || isStarting" @click="handleStart">
                    <UiSpinner v-if="isStarting" />
                    {{ $t('instance.import.start') }}
                </UiButton>
            </UiDialogFooter>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
import { filesize } from 'filesize';

import { ICircleAlert, IUser, IUserPlus, IUserX } from '#components';
import type { ImportApp } from '#shared/utils/importApps';

type ImportUser = {
    id: string;
    username: string;
    files: number;
    links: number;
    notes: number;
    match: { id: string; username: string } | null;
};

type ImportPreview = {
    app: ImportApp;
    version: string | null;
    files: number;
    found: number;
    size: number;
    folders: number;
    links: number;
    notes: number;
    users: ImportUser[];
};

type ImportTarget = { id: string; username: string } | 'new' | null;

const props = defineProps<{
    storage: { id: string | null; name: string; importPath?: string } | null;
}>();

const emit = defineEmits<{ started: [] }>();

const open = defineModel<boolean>('open', { required: true });

const UPLOADS_FOLDER = 'uploads';
const XBACKBONE_DATABASE = 'resources/database/xbackbone.db';
const CHIBISAFE_DATABASE = 'database/database.sqlite';

const { $toast } = useNuxtApp();
const { t, locale } = useI18n();
const { copy } = useClipboard({ legacy: true });

const sourceInput = ref<HTMLInputElement>();
const sourceId = ref<string | null>(null);
const preview = ref<ImportPreview | null>(null);
const targets = ref<Record<string, ImportTarget>>({});
const error = ref('');
const isReading = ref(false);
const isStarting = ref(false);

const importPath = computed(() => props.storage?.importPath);

const canStart = computed(
    () =>
        !!preview.value &&
        Object.values(targets.value).some(Boolean) &&
        !!(preview.value.found || preview.value.links || preview.value.notes),
);

const formatList = (items: string[], type: Intl.ListFormatType) =>
    new Intl.ListFormat(locale.value, { type, style: type === 'unit' ? 'short' : 'long' }).format(
        items,
    );

const countsOf = (counts: { folders?: number; links: number; notes: number }) =>
    [
        counts.folders && t('instance.appImport.folderCount', counts.folders),
        counts.links && t('instance.appImport.linkCount', counts.links),
        counts.notes && t('instance.appImport.noteCount', counts.notes),
    ].filter((item): item is string => !!item);

const extras = computed(() => (preview.value ? countsOf(preview.value) : []));

const userContents = (user: ImportUser) =>
    formatList([t('instance.appImport.fileCount', user.files), ...countsOf(user)], 'unit');

const targetId = (user: ImportUser) => {
    const target = targets.value[user.id];
    return target && target !== 'new' ? target.id : null;
};

const targetLabel = (user: ImportUser) => {
    const target = targets.value[user.id];
    if (target === 'new') return t('instance.appImport.newAccount');
    return target ? target.username : t('instance.appImport.skipAccount');
};

const targetIcon = (user: ImportUser) => {
    const target = targets.value[user.id];
    if (target === 'new') return IUserPlus;
    return target ? IUser : IUserX;
};

const handleTarget = (user: ImportUser, target: ImportTarget, close: () => void) => {
    targets.value[user.id] = target;
    close();
};

const loadPreview = async () => {
    isReading.value = true;
    error.value = '';

    try {
        preview.value = await $fetch<ImportPreview>('/api/storages/import/apps/preview', {
            method: 'POST',
            body: { sourceId: sourceId.value, storageId: props.storage!.id },
        });

        targets.value = Object.fromEntries(
            preview.value.users.map((user) => [
                user.id,
                targets.value[user.id] !== undefined
                    ? targets.value[user.id]!
                    : (user.match ?? 'new'),
            ]),
        );
    } catch (fetchError) {
        error.value = getErrorMessage(fetchError);
    }

    isReading.value = false;
};

const handleChooseSource = async (event: Event) => {
    const input = event.target as HTMLInputElement;
    const file = input.files?.[0];
    input.value = '';

    if (!file) return;

    isReading.value = true;
    error.value = '';
    preview.value = null;
    targets.value = {};

    try {
        sourceId.value = (
            await $fetch<{ id: string }>('/api/storages/import/apps/source', {
                method: 'POST',
                body: file,
            })
        ).id;
    } catch (fetchError) {
        sourceId.value = null;
        error.value = getErrorMessage(fetchError);
        isReading.value = false;
        return;
    }

    await loadPreview();
};

const handleCopyPath = async () => {
    await copy(importPath.value!);
    $toast.success(t('instance.import.pathCopied'));
};

const handleStart = async () => {
    isStarting.value = true;

    try {
        const result = await withVerification((verificationData) =>
            $fetch('/api/storages/import/apps', {
                method: 'POST',
                body: {
                    sourceId: sourceId.value,
                    storageId: props.storage!.id,
                    users: Object.fromEntries(
                        Object.entries(targets.value).map(([id, target]) => [
                            id,
                            target && target !== 'new' ? target.id : target,
                        ]),
                    ),
                    verificationData,
                },
            }),
        );

        if (result) {
            emit('started');
            open.value = false;
            $toast.success(t('instance.appImport.started'));
        }
    } catch (fetchError) {
        $toast.error(getErrorMessage(fetchError));
    }

    isStarting.value = false;
};

watch(open, (value) => {
    if (!value) return;

    sourceId.value = null;
    preview.value = null;
    targets.value = {};
    error.value = '';
});
</script>
