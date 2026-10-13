<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>Upload token</UiCardTitle>
            <UiCardDescription>
                Used by ShareX and the upload scripts. It can only upload files and shorten links,
                but keep it private.
            </UiCardDescription>
            <UiCardAction>
                <UiButton variant="outline" @click="handleRegenerate">
                    <IRefreshCw />
                    Regenerate
                </UiButton>
            </UiCardAction>
        </UiCardHeader>
        <UiCardContent class="flex gap-2">
            <PasswordInput
                :model-value="uploadToken?.token"
                readonly
                aria-label="Upload token"
                class="font-mono"
            />
            <UiButton
                variant="outline"
                size="icon"
                aria-label="Copy token"
                :disabled="!uploadToken"
                @click="handleCopy"
            >
                <ICopy />
            </UiButton>
        </UiCardContent>
    </UiCard>

    <UiCard v-if="canUpload">
        <UiCardHeader>
            <UiCardTitle>Upload options</UiCardTitle>
            <UiCardDescription>
                Applied to every config and script you download below.
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiFieldGroup>
                <UiField>
                    <UiFieldLabel>File name</UiFieldLabel>
                    <UiTabs v-model="settings.fileNameType">
                        <UiTabsList class="w-full">
                            <UiTabsTrigger
                                v-for="type in ['Random', 'UUID', 'Original']"
                                :key="type"
                                :value="type"
                            >
                                {{ type }}
                            </UiTabsTrigger>
                        </UiTabsList>
                    </UiTabs>
                </UiField>
                <div class="grid grid-cols-1 gap-4 sm:grid-cols-2">
                    <UiField>
                        <UiFieldLabel>Expiration</UiFieldLabel>
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
                                    {{ option.label }}
                                </UiSelectItem>
                            </UiSelectContent>
                        </UiSelect>
                    </UiField>
                    <UiField>
                        <UiFieldLabel>Compression</UiFieldLabel>
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
                                    {{ option.label }}
                                </UiSelectItem>
                            </UiSelectContent>
                        </UiSelect>
                    </UiField>
                    <UiField>
                        <UiFieldLabel>Max views</UiFieldLabel>
                        <UiNumberField v-model="settings.maxViews" :min="0">
                            <UiNumberFieldContent>
                                <UiNumberFieldDecrement />
                                <UiNumberFieldInput />
                                <UiNumberFieldIncrement />
                            </UiNumberFieldContent>
                        </UiNumberField>
                    </UiField>
                    <UiField>
                        <UiFieldLabel for="upload-password">Password</UiFieldLabel>
                        <PasswordInput
                            id="upload-password"
                            v-model="settings.password"
                            placeholder="Optional"
                        />
                    </UiField>
                </div>
                <UiField>
                    <UiFieldLabel>Folder</UiFieldLabel>
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
                    Download
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
const { copy } = useClipboard({ legacy: true });

const { data: uploadToken } = useFetch('/api/users/@me/upload-token', { server: false });

const settings = reactive({
    fileNameType: 'Random',
    expiration: 'Never',
    compression: 0,
    maxViews: 0,
    password: '',
    folderId: null as string | null,
});

const canUpload = computed(() => canUploadFiles(authUser.value));
const canShorten = computed(() => canShortenLinks(authUser.value));

const linkStep = computed(() =>
    canShorten.value ? ['Run it with --link and a URL to shorten the link.'] : [],
);

const integrations = computed(() => [
    ...(canUpload.value
        ? [
              {
                  name: 'ShareX',
                  description: 'A custom uploader for ShareX on Windows.',
                  fileName: 'fileship.sxcu',
                  steps: [
                      'Download the config and open it, ShareX imports it as a custom uploader.',
                      'In ShareX, set Destinations → Image, Text and File uploader to Custom uploader.',
                  ],
                  build: buildShareXConfig,
              },
          ]
        : []),
    ...(canShorten.value
        ? [
              {
                  name: 'ShareX URL shortener',
                  description: 'Shortens links from ShareX with your short links.',
                  fileName: 'fileship-shortener.sxcu',
                  steps: [
                      'Download the config and open it, ShareX imports it as a custom URL shortener.',
                      'In ShareX, set Destinations → URL shortener to Custom URL shortener.',
                  ],
                  build: buildShareXShortenerConfig,
              },
          ]
        : []),
    ...(canUpload.value
        ? [
              {
                  name: 'macOS',
                  description:
                      'A shell script that uploads a screenshot or files and copies the link.',
                  fileName: 'fileship.sh',
                  steps: [
                      'Download the script and make it executable with chmod +x fileship.sh.',
                      'Run it without arguments to capture a region, or pass files to upload them.',
                      ...linkStep.value,
                      'For a keyboard shortcut, run it from a Shortcuts "Run Shell Script" action and allow screen recording when asked.',
                  ],
                  build: (options: IntegrationOptions) => buildUnixScript('macos', options),
              },
              {
                  name: 'Linux',
                  description:
                      'A shell script that uploads a screenshot or files and copies the link.',
                  fileName: 'fileship.sh',
                  steps: [
                      'Install curl, a screenshot tool (grim and slurp, flameshot, maim, gnome-screenshot or spectacle) and wl-clipboard or xclip.',
                      'Download the script and make it executable with chmod +x fileship.sh.',
                      'Run it without arguments to capture a region, or pass files to upload them. Bind it to a shortcut in your desktop settings.',
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
    $toast.success('Upload token copied to clipboard');
};

const handleRegenerate = () =>
    askConfirmation({
        title: 'Regenerate upload token?',
        description: 'Configs and scripts that use the current token will stop working.',
        confirmText: 'Regenerate',
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
                    $toast.success('Upload token regenerated');
                }
            } catch (error: any) {
                $toast.error(getErrorMessage(error));
            }
        },
    });
</script>
