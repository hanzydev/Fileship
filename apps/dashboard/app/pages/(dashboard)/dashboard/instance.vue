<template>
    <Head>
        <Title>Instance</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-6 p-4 pt-0">
        <PageHeader
            title="Instance"
            description="Settings for everyone on this Fileship instance. Values set here override the environment."
        />

        <div class="grid items-start gap-6 lg:grid-cols-2">
            <template v-if="!data">
                <UiCard v-for="i in 4" :key="i">
                    <UiCardHeader>
                        <UiSkeleton class="h-5 w-32" />
                        <UiSkeleton class="h-4 w-64" />
                    </UiCardHeader>
                    <UiCardContent class="flex flex-col gap-4">
                        <UiSkeleton v-for="j in 2" :key="j" class="h-9 w-full" />
                    </UiCardContent>
                </UiCard>
            </template>

            <template v-else>
                <UiCard v-for="section in sections" :key="section.title">
                    <form class="contents" @submit.prevent="handleSave(section)">
                        <UiCardHeader>
                            <UiCardTitle>{{ section.title }}</UiCardTitle>
                            <UiCardDescription>{{ section.description }}</UiCardDescription>
                        </UiCardHeader>
                        <UiCardContent>
                            <UiFieldGroup>
                                <template v-for="(field, index) in section.fields" :key="field.key">
                                    <UiFieldSeparator v-if="index && field.type === 'switch'" />

                                    <UiField
                                        v-if="field.type === 'switch'"
                                        orientation="horizontal"
                                        class="has-[>[data-slot=field-content]]:items-center"
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

                                    <UiField v-else>
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
            </template>
        </div>
    </div>
</template>

<script setup lang="ts">
type FieldType = 'text' | 'secret' | 'number' | 'switch' | 'select' | 'permissions';

type SettingField = {
    key: InstanceSettingKey;
    label: string;
    type: FieldType;
    description?: string;
    min?: number;
    max?: number;
    options?: { value: string; label: string }[];
};

type SettingSection = {
    title: string;
    description: string;
    fields: SettingField[];
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
                min: 1,
                max: 100,
                description: 'Large files are uploaded in pieces of this size.',
            },
            {
                key: 'avatarMaxSize',
                label: 'Avatar size limit (MB)',
                type: 'number',
                min: 1,
                max: 100,
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
            { key: 'aiQueueConcurrency', label: 'Parallel jobs', type: 'number', min: 1, max: 8 },
            {
                key: 'aiInferenceThreads',
                label: 'Inference threads',
                type: 'number',
                min: 1,
                max: 64,
                description: 'Applies after a restart.',
            },
            { key: 'aiVideoMaxFrames', label: 'Video frames', type: 'number', min: 1, max: 120 },
            {
                key: 'aiVideoSampleIntervalSeconds',
                label: 'Video sample interval (seconds)',
                type: 'number',
                min: 1,
                max: 3600,
            },
            {
                key: 'aiJobLeaseMs',
                label: 'Job timeout (ms)',
                type: 'number',
                min: 60_000,
                description: 'A job running longer than this is retried.',
            },
            { key: 'aiJobDequeueRetries', label: 'Queue retries', type: 'number', min: 1, max: 50 },
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

definePageMeta({
    layout: 'dashboard',
    middleware: 'super-admin-only',
});
</script>
