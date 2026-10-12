<template>
    <Head>
        <Title>Instance</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader
            title="Instance"
            description="Settings for everyone on this Fileship instance. Values set here override the environment."
        />

        <div class="grid items-start gap-4 lg:grid-cols-2">
            <template v-if="!data">
                <div v-for="i in 2" :key="i" class="flex flex-col gap-4">
                    <UiCard v-for="j in 3" :key="j">
                        <UiCardHeader>
                            <UiSkeleton class="h-5 w-32" />
                            <UiSkeleton class="h-4 w-64" />
                        </UiCardHeader>
                        <UiCardContent class="flex flex-col gap-4">
                            <UiSkeleton v-for="k in 2" :key="k" class="h-9 w-full" />
                        </UiCardContent>
                    </UiCard>
                </div>
            </template>

            <template v-else>
                <div
                    v-for="(column, columnIndex) in columns"
                    :key="columnIndex"
                    class="flex flex-col gap-4"
                >
                    <UiCard v-for="section in column" :key="section.title">
                        <form class="contents" @submit.prevent="handleSave(section)">
                            <UiCardHeader>
                                <UiCardTitle>{{ section.title }}</UiCardTitle>
                                <UiCardDescription>{{ section.description }}</UiCardDescription>
                            </UiCardHeader>
                            <UiCardContent>
                                <UiFieldGroup class="grid gap-x-4 sm:grid-cols-2">
                                    <template
                                        v-for="(field, index) in section.fields"
                                        :key="field.key"
                                    >
                                        <UiFieldSeparator
                                            v-if="index && field.type === 'switch'"
                                            class="sm:col-span-2"
                                        />

                                        <UiField
                                            v-if="field.type === 'switch'"
                                            orientation="horizontal"
                                            class="has-[>[data-slot=field-content]]:items-center sm:col-span-2"
                                        >
                                            <UiFieldContent>
                                                <UiFieldLabel :for="field.key">
                                                    {{ field.label }}
                                                </UiFieldLabel>
                                                <UiFieldDescription>
                                                    {{ field.description }}
                                                </UiFieldDescription>
                                                <UiFieldDescription v-if="isOverridden(field.key)">
                                                    Default: {{ formatDefault(field) }}
                                                </UiFieldDescription>
                                            </UiFieldContent>
                                            <UiButton
                                                v-if="isOverridden(field.key)"
                                                type="button"
                                                variant="ghost"
                                                size="icon-sm"
                                                aria-label="Reset to default"
                                                @click="handleReset(field.key)"
                                            >
                                                <IRotateCcw />
                                            </UiButton>
                                            <UiSwitch
                                                :id="field.key"
                                                v-model="form[field.key] as boolean"
                                            />
                                        </UiField>

                                        <UiField v-else :class="!field.half && 'sm:col-span-2'">
                                            <div class="flex items-center justify-between gap-2">
                                                <UiFieldLabel :for="field.key">
                                                    {{ field.label }}
                                                </UiFieldLabel>
                                                <UiTooltip v-if="isOverridden(field.key)">
                                                    <UiTooltipTrigger as-child>
                                                        <UiButton
                                                            type="button"
                                                            variant="ghost"
                                                            size="icon-xs"
                                                            aria-label="Reset to default"
                                                            @click="handleReset(field.key)"
                                                        >
                                                            <IRotateCcw />
                                                        </UiButton>
                                                    </UiTooltipTrigger>
                                                    <UiTooltipContent>
                                                        Reset to {{ formatDefault(field) }}
                                                    </UiTooltipContent>
                                                </UiTooltip>
                                            </div>

                                            <UiInput
                                                v-if="field.type === 'text'"
                                                :id="field.key"
                                                v-model="form[field.key] as string"
                                            />

                                            <PasswordInput
                                                v-else-if="field.type === 'secret'"
                                                :id="field.key"
                                                v-model="secret"
                                                autocomplete="off"
                                                :placeholder="
                                                    data.turnstileSecretKeySet
                                                        ? 'Leave empty to keep'
                                                        : ''
                                                "
                                            />

                                            <UiNumberField
                                                v-else-if="field.type === 'number'"
                                                :id="field.key"
                                                v-model="form[field.key] as number"
                                                :min="field.min"
                                                :max="field.max"
                                            >
                                                <UiNumberFieldContent>
                                                    <UiNumberFieldDecrement />
                                                    <UiNumberFieldInput />
                                                    <UiNumberFieldIncrement />
                                                </UiNumberFieldContent>
                                            </UiNumberField>

                                            <UiSelect
                                                v-else-if="field.type === 'select'"
                                                :model-value="String(form[field.key])"
                                                @update:model-value="
                                                    handleSelect(field.key, $event as string)
                                                "
                                            >
                                                <UiSelectTrigger :id="field.key" class="w-full">
                                                    <UiSelectValue />
                                                </UiSelectTrigger>
                                                <UiSelectContent>
                                                    <UiSelectItem
                                                        v-for="option in field.options"
                                                        :key="option.value"
                                                        :value="option.value"
                                                    >
                                                        {{ option.label }}
                                                    </UiSelectItem>
                                                </UiSelectContent>
                                            </UiSelect>

                                            <div
                                                v-else-if="field.type === 'permissions'"
                                                class="flex flex-col gap-3"
                                            >
                                                <UiField
                                                    v-for="permission in registrablePermissions"
                                                    :key="permission.value"
                                                    orientation="horizontal"
                                                >
                                                    <UiCheckbox
                                                        :id="`permission-${permission.value}`"
                                                        :model-value="
                                                            form.registrationPermissions.includes(
                                                                permission.value,
                                                            )
                                                        "
                                                        @update:model-value="
                                                            handleTogglePermission(permission.value)
                                                        "
                                                    />
                                                    <UiFieldLabel
                                                        :for="`permission-${permission.value}`"
                                                    >
                                                        {{ permission.label }}
                                                    </UiFieldLabel>
                                                </UiField>
                                            </div>

                                            <UiFieldDescription v-if="field.description">
                                                {{ field.description }}
                                            </UiFieldDescription>
                                            <UiFieldError
                                                v-if="formErrors?.[field.key]?._errors?.length"
                                                :errors="formErrors[field.key]._errors"
                                            />
                                        </UiField>
                                    </template>
                                </UiFieldGroup>
                            </UiCardContent>
                            <UiCardFooter class="justify-end">
                                <UiButton
                                    type="submit"
                                    :disabled="!isDirty(section) || savingSection === section.title"
                                >
                                    <UiSpinner v-if="savingSection === section.title" />
                                    Save
                                </UiButton>
                            </UiCardFooter>
                        </form>
                    </UiCard>
                </div>
            </template>
        </div>

        <UiCard>
            <UiCardHeader>
                <UiCardTitle>Storage</UiCardTitle>
                <UiCardDescription>
                    Where uploads and backups are kept. New ones go to the active storage, existing
                    ones stay where they were written.
                </UiCardDescription>
                <UiCardAction>
                    <UiButton @click="handleOpenStorage()">
                        <IPlus />
                        Add storage
                    </UiButton>
                </UiCardAction>
            </UiCardHeader>
            <UiCardContent class="flex flex-col gap-2">
                <div v-if="migration" class="flex flex-col gap-2 pb-2">
                    <div class="flex items-center justify-between gap-2">
                        <TextBlock variant="small">Moving files and backups</TextBlock>
                        <TextBlock variant="muted" class="text-xs">
                            {{ migration.done }} / {{ migration.total }}
                        </TextBlock>
                    </div>
                    <UiProgress :model-value="(migration.done / (migration.total || 1)) * 100" />
                </div>

                <UiCard v-for="storage in storageRows" :key="storage.id ?? 'local'" size="sm">
                    <UiCardContent class="flex min-h-8 items-center gap-3">
                        <component
                            :is="storageIcons[storage.type]"
                            class="size-4 shrink-0 text-muted-foreground"
                        />
                        <div class="flex min-w-0 flex-1 flex-col">
                            <div class="flex min-w-0 items-center gap-2">
                                <TextBlock variant="small" class="truncate">
                                    {{ storage.name }}
                                </TextBlock>
                                <UiBadge variant="secondary">
                                    {{ storageTypeLabels[storage.type] }}
                                </UiBadge>
                                <UiBadge v-if="storage.id === storages!.activeStorageId">
                                    Active
                                </UiBadge>
                            </div>
                            <TextBlock variant="muted" class="text-xs">
                                {{ storage.files }} files, {{ storage.backups }} backups
                            </TextBlock>
                        </div>
                        <UiButton
                            v-if="storage.id !== storages!.activeStorageId"
                            variant="outline"
                            size="sm"
                            :disabled="activatingStorage === (storage.id ?? 'local')"
                            @click="handleActivateStorage(storage)"
                        >
                            <UiSpinner v-if="activatingStorage === (storage.id ?? 'local')" />
                            Set active
                        </UiButton>
                        <UiButton
                            v-else-if="hasDataElsewhere && !migration"
                            variant="outline"
                            size="sm"
                            @click="handleMigrateStorage(storage)"
                        >
                            Move everything here
                        </UiButton>
                        <UiDropdownMenu v-if="storage.id">
                            <UiDropdownMenuTrigger as-child>
                                <UiButton variant="ghost" size="icon-sm">
                                    <IEllipsis />
                                </UiButton>
                            </UiDropdownMenuTrigger>
                            <UiDropdownMenuContent align="end" class="w-56">
                                <UiDropdownMenuItem @select="handleOpenStorage(storage)">
                                    <IPencil />
                                    Edit
                                </UiDropdownMenuItem>
                                <UiDropdownMenuSeparator />
                                <UiDropdownMenuItem
                                    variant="destructive"
                                    :disabled="
                                        storage.id === storages!.activeStorageId ||
                                        !!storage.files ||
                                        !!storage.backups
                                    "
                                    @select="handleDeleteStorage(storage)"
                                >
                                    <ITrash2 />
                                    Delete
                                </UiDropdownMenuItem>
                            </UiDropdownMenuContent>
                        </UiDropdownMenu>
                    </UiCardContent>
                </UiCard>
            </UiCardContent>
        </UiCard>
    </div>

    <UiDialog v-model:open="storageDialog.open">
        <UiDialogContent class="sm:max-w-lg">
            <form class="flex flex-col gap-6" @submit.prevent="handleSaveStorage">
                <UiDialogHeader>
                    <UiDialogTitle>
                        {{ storageDialog.id ? 'Edit storage' : 'Add storage' }}
                    </UiDialogTitle>
                    <UiDialogDescription>
                        The connection is tested before saving. Secrets are stored encrypted.
                    </UiDialogDescription>
                </UiDialogHeader>

                <UiFieldGroup class="grid gap-x-4 sm:grid-cols-2">
                    <UiField :data-invalid="!!storageErrors?.name">
                        <UiFieldLabel for="storage-name">Name</UiFieldLabel>
                        <UiInput
                            id="storage-name"
                            v-model="storageForm.name"
                            placeholder="Media bucket"
                        />
                        <UiFieldError
                            v-if="storageErrors?.name"
                            :errors="storageErrors.name._errors"
                        />
                    </UiField>

                    <UiField>
                        <UiFieldLabel for="storage-type">Type</UiFieldLabel>
                        <UiSelect
                            :model-value="storageForm.type"
                            :disabled="!!storageDialog.id"
                            @update:model-value="handleStorageType($event as StorageType)"
                        >
                            <UiSelectTrigger id="storage-type" class="w-full">
                                <UiSelectValue />
                            </UiSelectTrigger>
                            <UiSelectContent>
                                <UiSelectItem
                                    v-for="type in Object.values(StorageType)"
                                    :key="type"
                                    :value="type"
                                >
                                    {{ storageTypeLabels[type] }}
                                </UiSelectItem>
                            </UiSelectContent>
                        </UiSelect>
                    </UiField>

                    <template v-for="field in storageFields[storageForm.type]" :key="field.key">
                        <UiField
                            v-if="field.type === 'switch'"
                            orientation="horizontal"
                            class="has-[>[data-slot=field-content]]:items-center sm:col-span-2"
                        >
                            <UiFieldContent>
                                <UiFieldLabel :for="`storage-${field.key}`">
                                    {{ field.label }}
                                </UiFieldLabel>
                                <UiFieldDescription>{{ field.description }}</UiFieldDescription>
                            </UiFieldContent>
                            <UiSwitch
                                :id="`storage-${field.key}`"
                                v-model="storageForm.config[field.key]"
                            />
                        </UiField>

                        <UiField
                            v-else
                            :class="!field.half && 'sm:col-span-2'"
                            :data-invalid="!!storageErrors?.config?.[field.key]"
                        >
                            <UiFieldLabel :for="`storage-${field.key}`">
                                {{ field.label }}
                            </UiFieldLabel>

                            <PasswordInput
                                v-if="field.type === 'secret'"
                                :id="`storage-${field.key}`"
                                v-model="storageForm.config[field.key]"
                                autocomplete="off"
                                :placeholder="
                                    storageDialog.secrets[field.key] ? 'Leave empty to keep' : ''
                                "
                            />

                            <UiTextarea
                                v-else-if="field.type === 'key'"
                                :id="`storage-${field.key}`"
                                v-model="storageForm.config[field.key]"
                                class="max-h-32 font-mono text-xs"
                                :placeholder="
                                    storageDialog.secrets[field.key]
                                        ? 'Leave empty to keep'
                                        : '-----BEGIN OPENSSH PRIVATE KEY-----'
                                "
                            />

                            <UiNumberField
                                v-else-if="field.type === 'number'"
                                :id="`storage-${field.key}`"
                                v-model="storageForm.config[field.key]"
                                :min="1"
                                :max="65535"
                                :format-options="{ useGrouping: false }"
                            >
                                <UiNumberFieldContent>
                                    <UiNumberFieldDecrement />
                                    <UiNumberFieldInput />
                                    <UiNumberFieldIncrement />
                                </UiNumberFieldContent>
                            </UiNumberField>

                            <UiInput
                                v-else
                                :id="`storage-${field.key}`"
                                v-model="storageForm.config[field.key]"
                                :placeholder="field.placeholder"
                            />

                            <UiFieldDescription v-if="field.description">
                                {{ field.description }}
                            </UiFieldDescription>
                            <UiFieldError
                                v-if="storageErrors?.config?.[field.key]?._errors?.length"
                                :errors="storageErrors.config[field.key]._errors"
                            />
                        </UiField>
                    </template>
                </UiFieldGroup>

                <UiDialogFooter>
                    <UiButton
                        type="button"
                        variant="outline"
                        :disabled="isTestingStorage"
                        @click="handleTestStorage"
                    >
                        <UiSpinner v-if="isTestingStorage" />
                        Test connection
                    </UiButton>
                    <UiButton type="submit" :disabled="isSavingStorage">
                        <UiSpinner v-if="isSavingStorage" />
                        Save
                    </UiButton>
                </UiDialogFooter>
            </form>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
import { ICloud, IHardDrive, IServer } from '#components';
import { StorageType } from '#shared/prisma/enums';

type FieldType = 'text' | 'secret' | 'number' | 'switch' | 'select' | 'permissions';

type SettingField = {
    key: InstanceSettingKey;
    label: string;
    type: FieldType;
    description?: string;
    min?: number;
    max?: number;
    options?: { value: string; label: string }[];
    half?: boolean;
};

type SettingSection = {
    title: string;
    description: string;
    fields: SettingField[];
};

type StorageField = {
    key: string;
    label: string;
    type: 'text' | 'secret' | 'key' | 'number' | 'switch';
    placeholder?: string;
    description?: string;
    half?: boolean;
};

const { $toast } = useNuxtApp();

const formErrors = ref();
const savingSection = ref<string>();
const secret = ref('');

const { data, refresh } = await useFetch('/api/settings');

const form = reactive({} as InstanceSettings);

watch(
    data,
    (value) => {
        if (value) Object.assign(form, structuredClone(toRaw(value.values)));
        secret.value = '';
    },
    { immediate: true },
);

const registrablePermissions = [
    { value: 'UploadFiles', label: 'Upload files' },
    { value: 'TakeNotes', label: 'Take notes' },
] as const;

const sections: SettingSection[] = [
    {
        title: 'General',
        description: 'How this instance presents itself.',
        fields: [
            { key: 'siteName', label: 'Site name', type: 'text' },
            {
                key: 'returnHttps',
                label: 'Link protocol',
                type: 'select',
                description: 'Protocol used in the links Fileship generates.',
                options: [
                    { value: 'auto', label: 'Same as the request' },
                    { value: 'true', label: 'Always HTTPS' },
                    { value: 'false', label: 'Always HTTP' },
                ],
            },
        ],
    },
    {
        title: 'Uploads',
        description: 'Limits and processing for uploaded files.',
        fields: [
            {
                key: 'fileChunkSize',
                label: 'Chunk size (MB)',
                type: 'number',
                min: 5,
                max: 100,
                description: 'Large files are uploaded in pieces of this size.',
                half: true,
            },
            {
                key: 'avatarMaxSize',
                label: 'Avatar size limit (MB)',
                type: 'number',
                min: 1,
                max: 100,
                half: true,
            },
            {
                key: 'removeExifData',
                label: 'Remove EXIF data',
                type: 'switch',
                description: 'Strip location and camera details from uploaded images.',
            },
        ],
    },
    {
        title: 'Registration',
        description: 'Let people create their own accounts from the login page.',
        fields: [
            {
                key: 'registrationEnabled',
                label: 'Allow registration',
                type: 'switch',
                description: 'Show a sign up link on the login page.',
            },
            { key: 'registrationPermissions', label: 'Permissions', type: 'permissions' },
            {
                key: 'registrationUsableSpace',
                label: 'Storage limit (MB)',
                type: 'number',
                min: -1,
                description: '-1 means unlimited.',
            },
        ],
    },
    {
        title: 'Turnstile',
        description: 'Cloudflare Turnstile protects the login and sign up forms from bots.',
        fields: [
            { key: 'turnstileSiteKey', label: 'Site key', type: 'text' },
            {
                key: 'turnstileSecretKey',
                label: 'Secret key',
                type: 'secret',
                description:
                    'Turnstile only turns on when both keys are set. A wrong secret blocks every login.',
            },
        ],
    },
    {
        title: 'AI',
        description: 'Captions, smart search and sensitive content detection.',
        fields: [
            {
                key: 'aiEnabled',
                label: 'AI features',
                type: 'switch',
                description: 'Process uploads with the local AI models.',
            },
            {
                key: 'aiQueueConcurrency',
                label: 'Parallel jobs',
                type: 'number',
                min: 1,
                max: 8,
                half: true,
            },
            {
                key: 'aiInferenceThreads',
                label: 'Inference threads',
                type: 'number',
                min: 1,
                max: 64,
                description: 'Applies after a restart.',
                half: true,
            },
            {
                key: 'aiVideoMaxFrames',
                label: 'Video frames',
                type: 'number',
                min: 1,
                max: 120,
                half: true,
            },
            {
                key: 'aiVideoSampleIntervalSeconds',
                label: 'Frame interval (seconds)',
                type: 'number',
                min: 1,
                max: 3600,
                half: true,
            },
            {
                key: 'aiJobLeaseMs',
                label: 'Job timeout (ms)',
                type: 'number',
                min: 60_000,
                description: 'A job running longer than this is retried.',
                half: true,
            },
            {
                key: 'aiJobDequeueRetries',
                label: 'Queue retries',
                type: 'number',
                min: 1,
                max: 50,
                half: true,
            },
        ],
    },
    {
        title: 'Logs and telemetry',
        description: 'What Fileship records and shares.',
        fields: [
            {
                key: 'logLevel',
                label: 'Log level',
                type: 'select',
                options: [
                    { value: '0', label: 'Nothing' },
                    { value: '1', label: 'Important actions' },
                    { value: '2', label: 'Everything' },
                ],
            },
            {
                key: 'telemetryEnabled',
                label: 'Anonymous telemetry',
                type: 'switch',
                description: 'Share anonymous usage statistics to help improve Fileship.',
            },
        ],
    },
];

const columns = [
    [sections[0]!, sections[1]!, sections[2]!],
    [sections[4]!, sections[3]!, sections[5]!],
];

const storageTypeLabels = {
    Local: 'Local',
    [StorageType.S3]: 'S3',
    [StorageType.SFTP]: 'SFTP',
    [StorageType.FTP]: 'FTP',
};

const storageIcons = {
    Local: IHardDrive,
    [StorageType.S3]: ICloud,
    [StorageType.SFTP]: IServer,
    [StorageType.FTP]: IServer,
};

const storageFields: Record<StorageType, StorageField[]> = {
    [StorageType.S3]: [
        {
            key: 'endpoint',
            label: 'Endpoint',
            type: 'text',
            placeholder: 'https://<account>.r2.cloudflarestorage.com',
            description: 'Leave empty for AWS. Needed for R2, MinIO and other S3 services.',
        },
        { key: 'region', label: 'Region', type: 'text', placeholder: 'us-east-1', half: true },
        { key: 'bucket', label: 'Bucket', type: 'text', half: true },
        { key: 'accessKeyId', label: 'Access key ID', type: 'text', half: true },
        { key: 'secretAccessKey', label: 'Secret access key', type: 'secret', half: true },
        {
            key: 'prefix',
            label: 'Prefix',
            type: 'text',
            placeholder: 'fileship',
            description: 'Optional folder inside the bucket.',
        },
        {
            key: 'forcePathStyle',
            label: 'Path-style URLs',
            type: 'switch',
            description: 'Needed by MinIO and some self-hosted S3 services.',
        },
    ],
    [StorageType.SFTP]: [
        { key: 'host', label: 'Host', type: 'text', half: true },
        { key: 'port', label: 'Port', type: 'number', half: true },
        { key: 'username', label: 'Username', type: 'text', half: true },
        { key: 'password', label: 'Password', type: 'secret', half: true },
        {
            key: 'privateKey',
            label: 'Private key',
            type: 'key',
            description: 'Use a password, a private key or both.',
        },
        { key: 'root', label: 'Folder', type: 'text', placeholder: '/srv/fileship' },
    ],
    [StorageType.FTP]: [
        { key: 'host', label: 'Host', type: 'text', half: true },
        { key: 'port', label: 'Port', type: 'number', half: true },
        { key: 'user', label: 'Username', type: 'text', half: true },
        { key: 'password', label: 'Password', type: 'secret', half: true },
        { key: 'root', label: 'Folder', type: 'text', placeholder: '/fileship' },
        {
            key: 'secure',
            label: 'Use TLS',
            type: 'switch',
            description: 'Connect with explicit FTPS.',
        },
    ],
};

const storageDefaults: Record<StorageType, Record<string, any>> = {
    [StorageType.S3]: {
        endpoint: '',
        region: 'us-east-1',
        bucket: '',
        prefix: '',
        accessKeyId: '',
        secretAccessKey: '',
        forcePathStyle: false,
    },
    [StorageType.SFTP]: {
        host: '',
        port: 22,
        username: '',
        password: '',
        privateKey: '',
        root: '/',
    },
    [StorageType.FTP]: {
        host: '',
        port: 21,
        user: '',
        password: '',
        secure: false,
        root: '/',
    },
};

const migration = useStorageMigration();
const storageErrors = ref();
const isTestingStorage = ref(false);
const isSavingStorage = ref(false);
const activatingStorage = ref<string>();

const storageDialog = reactive({
    open: false,
    id: null as string | null,
    secrets: {} as Record<string, boolean>,
});

const storageForm = reactive({
    name: '',
    type: StorageType.S3 as StorageType,
    config: {} as Record<string, any>,
});

const { data: storages, refresh: refreshStorages } = await useFetch('/api/storages');

migration.value = storages.value?.migration ?? null;

watch(migration, (value, oldValue) => {
    if (!value && oldValue) refreshStorages();
});

type StorageRow = NonNullable<typeof storages.value>['items'][number];

const storageRows = computed(() => [
    { id: null, name: 'Data directory', type: 'Local' as const, ...storages.value!.local },
    ...storages.value!.items,
]);

const hasDataElsewhere = computed(() =>
    storageRows.value.some(
        (storage) =>
            storage.id !== storages.value!.activeStorageId && (storage.files || storage.backups),
    ),
);

const isOverridden = (key: InstanceSettingKey) => !!data.value?.overridden.includes(key);

const formatDefault = (field: SettingField) => {
    const value = data.value!.defaults[field.key];

    if (field.type === 'secret') return 'none';
    if (typeof value === 'boolean') return value ? 'On' : 'Off';
    if (field.options) return field.options.find((o) => o.value === String(value))?.label;
    if (Array.isArray(value)) return value.length ? value.join(', ') : 'none';

    return String(value || 'empty');
};

const getChanges = (section: SettingSection) => {
    const changes: Record<string, unknown> = {};

    for (const field of section.fields) {
        if (field.type === 'secret') {
            if (secret.value.trim()) changes[field.key] = secret.value.trim();
        } else if (
            JSON.stringify(form[field.key]) !== JSON.stringify(data.value!.values[field.key])
        ) {
            changes[field.key] = form[field.key];
        }
    }

    return changes;
};

const isDirty = (section: SettingSection) => !!Object.keys(getChanges(section)).length;

const handleSelect = (key: InstanceSettingKey, value: string) => {
    (form as any)[key] = key === 'logLevel' ? Number(value) : value;
};

const handleTogglePermission = (permission: string) => {
    form.registrationPermissions = form.registrationPermissions.includes(permission as never)
        ? form.registrationPermissions.filter((p) => p !== permission)
        : [...form.registrationPermissions, permission as never];
};

const handleSave = async (section: SettingSection) => {
    savingSection.value = section.title;
    formErrors.value = null;

    try {
        const result = await withVerification((verificationData) =>
            $fetch('/api/settings', {
                method: 'PATCH',
                body: { ...getChanges(section), verificationData },
            }),
        );

        if (result) {
            await refresh();
            $toast.success(`${section.title} settings saved`);
        }
    } catch (error: any) {
        formErrors.value = error.data?.data?.formErrors;
        if (!formErrors.value) $toast.error(getErrorMessage(error));
    }

    savingSection.value = undefined;
};

const handleReset = async (key: InstanceSettingKey) => {
    try {
        const result = await withVerification((verificationData) =>
            $fetch('/api/settings', {
                method: 'PATCH',
                body: { [key]: null, verificationData },
            }),
        );

        if (result) {
            await refresh();
            $toast.success('Reset to default');
        }
    } catch (error) {
        $toast.error(getErrorMessage(error));
    }
};

const handleOpenStorage = (storage?: StorageRow) => {
    storageErrors.value = null;
    storageDialog.id = storage?.id ?? null;
    storageDialog.secrets = storage?.secrets ?? {};
    storageForm.name = storage?.name ?? '';
    storageForm.type = storage?.type ?? StorageType.S3;
    storageForm.config = { ...storageDefaults[storageForm.type], ...storage?.config };
    storageDialog.open = true;
};

const handleStorageType = (type: StorageType) => {
    storageForm.type = type;
    storageForm.config = { ...storageDefaults[type] };
};

const handleTestStorage = async () => {
    isTestingStorage.value = true;
    storageErrors.value = null;

    try {
        await $fetch('/api/storages/test', {
            method: 'POST',
            body: { ...storageForm, id: storageDialog.id ?? undefined },
        });

        $toast.success('Connection works');
    } catch (error: any) {
        storageErrors.value = error.data?.data?.formErrors;
        if (!storageErrors.value) $toast.error(getErrorMessage(error));
    }

    isTestingStorage.value = false;
};

const handleSaveStorage = async () => {
    isSavingStorage.value = true;
    storageErrors.value = null;

    try {
        const result = await withVerification((verificationData) =>
            storageDialog.id
                ? $fetch(`/api/storages/${storageDialog.id}`, {
                      method: 'PATCH',
                      body: {
                          name: storageForm.name,
                          config: storageForm.config,
                          verificationData,
                      },
                  })
                : $fetch('/api/storages', {
                      method: 'POST',
                      body: { ...storageForm, verificationData },
                  }),
        );

        if (result) {
            storageDialog.open = false;
            await refreshStorages();
            $toast.success('Storage saved');
        }
    } catch (error: any) {
        storageErrors.value = error.data?.data?.formErrors;
        if (!storageErrors.value) $toast.error(getErrorMessage(error));
    }

    isSavingStorage.value = false;
};

const handleActivateStorage = async (storage: { id: string | null; name: string }) => {
    activatingStorage.value = storage.id ?? 'local';

    try {
        const result = await withVerification((verificationData) =>
            $fetch('/api/storages/active', {
                method: 'PUT',
                body: { id: storage.id, verificationData },
            }),
        );

        if (result) {
            await refreshStorages();
            $toast.success(`New files now go to ${storage.name}`);
        }
    } catch (error) {
        $toast.error(getErrorMessage(error));
    }

    activatingStorage.value = undefined;
};

const handleMigrateStorage = async (storage: { name: string }) => {
    await askConfirmation({
        title: `Move everything to ${storage.name}?`,
        description:
            'Every file and backup on other storages is copied here and then removed from where it was. This can take a while.',
        confirmText: 'Move',
        action: async () => {
            try {
                const result = await withVerification((verificationData) =>
                    $fetch('/api/storages/migrate', {
                        method: 'POST',
                        body: { verificationData },
                    }),
                );

                if (result) $toast.success('Moving files in the background');
            } catch (error) {
                $toast.error(getErrorMessage(error));
            }
        },
    });
};

const handleDeleteStorage = async (storage: StorageRow) => {
    await askConfirmation({
        title: `Delete ${storage.name}?`,
        description: 'Fileship forgets this storage. Nothing is deleted from the storage itself.',
        confirmText: 'Delete',
        destructive: true,
        action: async () => {
            try {
                const result = await withVerification((verificationData) =>
                    $fetch(`/api/storages/${storage.id}`, {
                        method: 'DELETE',
                        body: { verificationData },
                    }),
                );

                if (result) {
                    await refreshStorages();
                    $toast.success('Storage deleted');
                }
            } catch (error) {
                $toast.error(getErrorMessage(error));
            }
        },
    });
};

definePageMeta({
    layout: 'dashboard',
    middleware: 'super-admin-only',
});
</script>
