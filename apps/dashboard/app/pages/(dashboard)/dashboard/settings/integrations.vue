<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('integrations.uploadToken') }}</UiCardTitle>
            <UiCardDescription>
                {{ $t('integrations.uploadTokenDescription') }}
            </UiCardDescription>
            <UiCardAction>
                <UiButton variant="outline" @click="handleRegenerate">
                    <IRefreshCw />
                    {{ $t('integrations.regenerate') }}
                </UiButton>
            </UiCardAction>
        </UiCardHeader>
        <UiCardContent class="flex gap-2">
            <PasswordInput
                :model-value="uploadToken?.token"
                readonly
                :aria-label="$t('integrations.uploadToken')"
                class="font-mono"
            />
            <UiButton
                variant="outline"
                size="icon"
                :aria-label="$t('integrations.copyToken')"
                :disabled="!uploadToken"
                @click="handleCopy"
            >
                <ICopy />
            </UiButton>
        </UiCardContent>
    </UiCard>

    <UiCard v-if="canUpload">
        <UiCardHeader>
            <UiCardTitle>{{ $t('integrations.uploadOptions') }}</UiCardTitle>
            <UiCardDescription>
                {{ $t('integrations.uploadOptionsDescription') }}
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiFieldGroup>
                <UiField>
                    <UiFieldLabel>{{ $t('integrations.fileName') }}</UiFieldLabel>
                    <UiTabs v-model="settings.fileNameType">
                        <UiTabsList class="w-full">
                            <UiTabsTrigger
                                v-for="(label, type) in FILE_NAME_TYPES"
                                :key="type"
                                :value="type"
                            >
                                {{ $t(label) }}
                            </UiTabsTrigger>
                        </UiTabsList>
                    </UiTabs>
                </UiField>
                <div class="grid grid-cols-1 gap-4 sm:grid-cols-2">
                    <UiField>
                        <UiFieldLabel>{{ $t('integrations.expiration') }}</UiFieldLabel>
                        <UiSelect v-model="settings.expiration">
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
                        <UiFieldLabel>{{ $t('integrations.compression') }}</UiFieldLabel>
                        <UiSelect v-model="settings.compression">
                            <UiSelectTrigger class="w-full">
                                <UiSelectValue />
                            </UiSelectTrigger>
                            <UiSelectContent>
                                <UiSelectItem
                                    v-for="option in COMPRESSIONS"
                                    :key="option.value"
                                    :value="option.value"
                                >
                                    {{ $t(option.label) }}
                                </UiSelectItem>
                            </UiSelectContent>
                        </UiSelect>
                    </UiField>
                    <UiField>
                        <UiFieldLabel>{{ $t('integrations.maxViews') }}</UiFieldLabel>
                        <UiNumberField v-model="settings.maxViews" :min="0">
                            <UiNumberFieldContent>
                                <UiNumberFieldDecrement />
                                <UiNumberFieldInput />
                                <UiNumberFieldIncrement />
                            </UiNumberFieldContent>
                        </UiNumberField>
                    </UiField>
                    <UiField>
                        <UiFieldLabel for="upload-password">
                            {{ $t('integrations.password') }}
                        </UiFieldLabel>
                        <PasswordInput
                            id="upload-password"
                            v-model="settings.password"
                            :placeholder="$t('integrations.optional')"
                        />
                    </UiField>
                </div>
                <UiField>
                    <UiFieldLabel>{{ $t('integrations.folder') }}</UiFieldLabel>
                    <FolderPicker v-model="settings.folderId" />
                </UiField>
            </UiFieldGroup>
        </UiCardContent>
    </UiCard>

    <UiCard v-for="integration in integrations" :key="integration.name">
        <UiCardHeader>
            <UiCardTitle>{{ integration.name }}</UiCardTitle>
            <UiCardDescription>{{ integration.description }}</UiCardDescription>
            <UiCardAction>
                <UiButton
                    variant="outline"
                    :disabled="!uploadToken"
                    @click="handleDownload(integration)"
                >
                    <IDownload />
                    {{ $t('integrations.download') }}
                </UiButton>
            </UiCardAction>
        </UiCardHeader>
        <UiCardContent>
            <ol class="flex list-decimal flex-col gap-1 pl-5 text-sm text-muted-foreground">
                <li v-for="step in integration.steps" :key="step">{{ step }}</li>
            </ol>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
const publicSettings = usePublicSettings();
const authUser = useAuthUser();
const requestUrl = useRequestURL();
const { $toast } = useNuxtApp();
const { t } = useI18n();
const { copy } = useClipboard({ legacy: true });

const { data: uploadToken } = useFetch('/api/users/@me/upload-token', { server: false });

const settings = reactive({
    fileNameType: 'Random',
    expiration: 'labels.expiration.never',
    compression: 0,
    maxViews: 0,
    password: '',
    folderId: null as string | null,
});

const FILE_NAME_TYPES = {
    Random: 'integrations.fileNameTypes.random',
    UUID: 'integrations.fileNameTypes.uuid',
    Original: 'integrations.fileNameTypes.original',
};

const canUpload = computed(() => canUploadFiles(authUser.value));
const canShorten = computed(() => canShortenLinks(authUser.value));

const linkStep = computed(() => (canShorten.value ? [t('integrations.steps.link')] : []));

const integrations = computed(() => [
    ...(canUpload.value
        ? [
              {
                  name: 'ShareX',
                  description: t('integrations.sharex.description'),
                  fileName: 'fileship.sxcu',
                  steps: [t('integrations.sharex.import'), t('integrations.sharex.destinations')],
                  build: buildShareXConfig,
              },
          ]
        : []),
    ...(canShorten.value
        ? [
              {
                  name: t('integrations.shortener.name'),
                  description: t('integrations.shortener.description'),
                  fileName: 'fileship-shortener.sxcu',
                  steps: [
                      t('integrations.shortener.import'),
                      t('integrations.shortener.destinations'),
                  ],
                  build: buildShareXShortenerConfig,
              },
          ]
        : []),
    ...(canUpload.value
        ? [
              {
                  name: 'macOS',
                  description: t('integrations.script.description'),
                  fileName: 'fileship.sh',
                  steps: [
                      t('integrations.steps.chmod'),
                      t('integrations.steps.run'),
                      ...linkStep.value,
                      t('integrations.steps.macosShortcut'),
                  ],
                  build: (options: IntegrationOptions) => buildUnixScript('macos', options),
              },
              {
                  name: 'Linux',
                  description: t('integrations.script.description'),
                  fileName: 'fileship.sh',
                  steps: [
                      t('integrations.steps.linuxInstall'),
                      t('integrations.steps.chmod'),
                      t('integrations.steps.linuxRun'),
                      ...linkStep.value,
                  ],
                  build: (options: IntegrationOptions) => buildUnixScript('linux', options),
              },
          ]
        : []),
]);

const fields = computed(() => {
    const expiration = EXPIRATIONS.find((e) => e.label === settings.expiration)!.value;

    return {
        fileNameType: settings.fileNameType,
        maxViews: settings.maxViews,
        compression: settings.compression,
        ...(expiration && { expiration }),
        ...(settings.password && { password: settings.password }),
        ...(settings.folderId && { folderId: settings.folderId }),
    };
});

const handleDownload = (integration: (typeof integrations.value)[number]) => {
    const content = integration.build({
        name: `${publicSettings.value.siteName} - ${integration.fileName.includes('shortener') ? 'URL Shortener' : 'File Uploader'}`,
        url: `${requestUrl.origin}/api/files`,
        linkUrl: canShorten.value ? `${requestUrl.origin}/api/links` : undefined,
        token: uploadToken.value!.token,
        fields: fields.value,
    });

    downloadFile(
        URL.createObjectURL(new Blob([content], { type: 'text/plain' })),
        integration.fileName,
    );
};

const handleCopy = () => {
    copy(uploadToken.value!.token);
    $toast.success(t('integrations.tokenCopied'));
};

const handleRegenerate = () =>
    askConfirmation({
        title: t('integrations.regenerateTitle'),
        description: t('integrations.regenerateDescription'),
        confirmText: t('integrations.regenerate'),
        action: async () => {
            try {
                const result = await withVerification((verificationData) =>
                    $fetch('/api/users/@me/upload-token', {
                        method: 'POST',
                        body: { verificationData },
                    }),
                );

                if (result) {
                    uploadToken.value = result;
                    $toast.success(t('integrations.tokenRegenerated'));
                }
            } catch (error: any) {
                $toast.error(getErrorMessage(error));
            }
        },
    });

definePageMeta({
    middleware: [
        () => {
            const user = useAuthUser();

            if (!canUploadFiles(user) && !canShortenLinks(user)) {
                return abortNavigation({
                    status: 403,
                    message: useNuxtApp().$i18n.t('auth.noPermission'),
                });
            }
        },
    ],
});
</script>
