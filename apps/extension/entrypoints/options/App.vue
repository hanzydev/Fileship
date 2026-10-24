<template>
    <div class="mx-auto flex min-h-svh w-full max-w-2xl flex-col gap-6 px-4 py-8">
        <header class="flex items-center gap-3">
            <img src="/icon/128.png" alt="Fileship" class="size-10" />
            <div class="flex flex-col gap-1">
                <TextBlock variant="h4">{{ $t('options.title') }}</TextBlock>
                <TextBlock variant="muted">{{ $t('options.description') }}</TextBlock>
            </div>
        </header>

        <UiCard>
            <UiCardHeader>
                <UiCardTitle>{{ $t('options.connection') }}</UiCardTitle>
                <UiCardDescription>{{ $t('options.connectionDescription') }}</UiCardDescription>
            </UiCardHeader>
            <UiCardContent>
                <form class="flex flex-col gap-6" @submit.prevent="handleSave">
                    <UiFieldGroup>
                        <UiField>
                            <UiFieldLabel for="instance-url">
                                {{ $t('options.serverAddress') }}
                            </UiFieldLabel>
                            <UiInput
                                id="instance-url"
                                v-model="instanceUrl"
                                :placeholder="$t('options.serverAddressPlaceholder')"
                                autocomplete="url"
                                spellcheck="false"
                                @input="savedMessage = ''"
                            />
                        </UiField>
                        <UiField>
                            <UiFieldLabel for="upload-token">
                                {{ $t('options.uploadToken') }}
                            </UiFieldLabel>
                            <PasswordInput
                                id="upload-token"
                                v-model="token"
                                autocomplete="off"
                                spellcheck="false"
                                class="font-mono"
                                @input="savedMessage = ''"
                            />
                            <UiFieldDescription>
                                {{ $t('options.uploadTokenDescription') }}
                            </UiFieldDescription>
                        </UiField>
                    </UiFieldGroup>

                    <UiAlert v-if="result" :variant="result.isError ? 'destructive' : 'default'">
                        <ICircleAlert v-if="result.isError" />
                        <ICircleCheck v-else />
                        <UiAlertTitle>{{ result.title }}</UiAlertTitle>
                        <UiAlertDescription>{{ result.description }}</UiAlertDescription>
                    </UiAlert>

                    <div class="flex flex-wrap items-center justify-end gap-2">
                        <UiButton
                            v-if="isConnected"
                            type="button"
                            variant="ghost"
                            class="sm:mr-auto"
                            :disabled="!!busy"
                            @click="handleDisconnect"
                        >
                            <IUnplug />
                            {{ $t('options.disconnect') }}
                        </UiButton>
                        <TextBlock
                            v-if="savedMessage"
                            variant="muted"
                            class="flex items-center gap-1.5"
                        >
                            <ICheck class="size-4" />
                            {{ savedMessage }}
                        </TextBlock>
                        <UiButton
                            type="button"
                            variant="outline"
                            :disabled="!!busy"
                            @click="handleTest"
                        >
                            <UiSpinner v-if="busy === 'test'" />
                            <IPlugZap v-else />
                            {{ $t('options.test') }}
                        </UiButton>
                        <UiButton type="submit" :disabled="!!busy">
                            <UiSpinner v-if="busy === 'save'" />
                            {{ $t('options.save') }}
                        </UiButton>
                    </div>
                </form>
            </UiCardContent>
        </UiCard>

        <UiCard v-if="settings">
            <UiCardHeader>
                <UiCardTitle>{{ $t('options.uploadOptions') }}</UiCardTitle>
                <UiCardDescription>{{ $t('options.uploadOptionsDescription') }}</UiCardDescription>
            </UiCardHeader>
            <UiCardContent>
                <UiFieldGroup>
                    <UiField>
                        <UiFieldLabel>{{ $t('options.fileName') }}</UiFieldLabel>
                        <UiTabs
                            :model-value="settings.fileNameType"
                            @update:model-value="
                                updateSettings({ fileNameType: $event as FileNameType })
                            "
                        >
                            <UiTabsList class="w-full">
                                <UiTabsTrigger
                                    v-for="type in FILE_NAME_TYPES"
                                    :key="type"
                                    :value="type"
                                >
                                    {{ $t(`labels.fileNameTypes.${type}`) }}
                                </UiTabsTrigger>
                            </UiTabsList>
                        </UiTabs>
                    </UiField>

                    <UiField>
                        <UiFieldLabel>{{ $t('options.expiration') }}</UiFieldLabel>
                        <UiSelect
                            :model-value="settings.expiration"
                            @update:model-value="updateSettings({ expiration: $event as number })"
                        >
                            <UiSelectTrigger class="w-full">
                                <UiSelectValue />
                            </UiSelectTrigger>
                            <UiSelectContent>
                                <UiSelectItem
                                    v-for="option in EXPIRATIONS"
                                    :key="option.value"
                                    :value="option.value"
                                >
                                    {{ $t(option.label) }}
                                </UiSelectItem>
                            </UiSelectContent>
                        </UiSelect>
                    </UiField>
                </UiFieldGroup>
            </UiCardContent>
        </UiCard>
    </div>
</template>

<script setup lang="ts">
type Result = { isError: boolean; title: string; description: string };

const settings = useSettings();
const instanceUrl = ref('');
const token = ref('');
const busy = ref<'test' | 'save' | null>(null);
const result = ref<Result | null>(null);
const savedMessage = ref('');

const isConnected = computed(() => !!settings.value?.instanceUrl && !!settings.value.token);

const describeConnection = (info: ConnectionInfo): Result => ({
    isError: false,
    title: t('options.connected', { siteName: info.siteName, version: info.version }),
    description: t(
        info.canUpload && info.canShorten
            ? 'options.canUploadAndShorten'
            : info.canUpload
              ? 'options.canUploadOnly'
              : 'options.canShortenOnly',
    ),
});

const describeFailure = (error: unknown): Result => ({
    isError: true,
    title: t('options.connectionFailed'),
    description: describeError(error),
});

settingsItem.getValue().then((value) => {
    instanceUrl.value = value.instanceUrl;
    token.value = value.token;
    if (value.connection) result.value = describeConnection(value.connection);
});

const readForm = (): Connection => {
    const url = normalizeInstanceUrl(instanceUrl.value);

    if (!url) throw new ExtensionError('errors.invalidAddress');
    if (!token.value.trim()) throw new ExtensionError('errors.tokenRequired');

    return { instanceUrl: url, token: token.value.trim(), language: getLanguage() };
};

const check = async (kind: 'test' | 'save') => {
    let connection: Connection;

    try {
        connection = readForm();
    } catch (error) {
        result.value = describeFailure(error);
        return null;
    }

    const granted = browser.permissions.request({
        origins: [originPattern(connection.instanceUrl)],
    });

    busy.value = kind;
    savedMessage.value = '';

    try {
        if (!(await granted))
            throw new ExtensionError('errors.permissionDenied', {
                origin: new URL(connection.instanceUrl).host,
            });

        const info = await testConnection(connection);

        result.value = describeConnection(info);
        return { connection, info };
    } catch (error) {
        result.value = describeFailure(error);
        return null;
    } finally {
        busy.value = null;
    }
};

const handleTest = () => check('test');

const handleSave = async () => {
    const previous = settings.value?.instanceUrl;
    const checked = await check('save');
    if (!checked) return;

    await updateSettings({
        instanceUrl: checked.connection.instanceUrl,
        token: checked.connection.token,
        connection: checked.info,
    });

    instanceUrl.value = checked.connection.instanceUrl;
    savedMessage.value = t('options.saved');

    if (previous && previous !== checked.connection.instanceUrl)
        browser.permissions.remove({ origins: [originPattern(previous)] });
};

const handleDisconnect = async () => {
    const previous = settings.value!.instanceUrl;

    await updateSettings({ instanceUrl: '', token: '', connection: null });
    await browser.permissions.remove({ origins: [originPattern(previous)] });

    instanceUrl.value = '';
    token.value = '';
    result.value = null;
    savedMessage.value = t('options.disconnected');
};
</script>
