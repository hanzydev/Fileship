<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>Upload token</UiCardTitle>
            <UiCardDescription>
                Used by ShareX and the upload scripts. It can only upload files, but keep it
                private.
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

    <UiCard>
        <form class="contents" @submit.prevent="handleDownload">
            <UiCardHeader>
                <UiCardTitle>ShareX</UiCardTitle>
                <UiCardDescription>
                    Generate a custom uploader config to upload straight from ShareX.
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
                            <UiFieldLabel for="sharex-password">Password</UiFieldLabel>
                            <PasswordInput
                                id="sharex-password"
                                v-model="settings.password"
                                placeholder="Optional"
                            />
                        </UiField>
                    </div>
                </UiFieldGroup>
            </UiCardContent>
            <UiCardFooter
                class="flex-col items-stretch justify-between gap-4 sm:flex-row sm:items-center"
            >
                <TextBlock variant="muted" class="text-xs">
                    The config contains your upload token. Do not share it.
                </TextBlock>
                <UiButton type="submit" :disabled="!uploadToken">
                    <IDownload />
                    Download config
                </UiButton>
            </UiCardFooter>
        </form>
    </UiCard>
</template>

<script setup lang="ts">
const publicSettings = usePublicSettings();
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
});

const handleDownload = () => {
    const config = {
        Version: '16.1.0',
        Name: `${publicSettings.value.siteName} - File Uploader`,
        DestinationType: 'ImageUploader, TextUploader, FileUploader',
        RequestMethod: 'POST',
        RequestURL: `${requestUrl.origin}/api/files`,
        Headers: {
            Authorization: `Bearer ${uploadToken.value!.token}`,
        },
        URL: '{json:url}',
        ErrorMessage: '{json:message}',
        Body: 'MultipartFormData',
        FileFormName: 'file',
        Arguments: {
            fileNameType: settings.fileNameType,
            password: settings.password || null,
            expiration: EXPIRATIONS.find((e) => e.label === settings.expiration)!.value,
            maxViews: settings.maxViews,
            compression: settings.compression,
        },
    };

    downloadFile(
        URL.createObjectURL(new Blob([JSON.stringify(config)], { type: 'application/json' })),
        'FileUploader.sxcu',
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
