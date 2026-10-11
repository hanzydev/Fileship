<template>
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
                            <UiInput
                                id="sharex-password"
                                v-model="settings.password"
                                type="password"
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
                    The config contains your session token. Do not share it.
                </TextBlock>
                <UiButton type="submit">
                    <IDownload />
                    Download config
                </UiButton>
            </UiCardFooter>
        </form>
    </UiCard>
</template>

<script setup lang="ts">
const runtimeConfig = useRuntimeConfig();
const requestUrl = useRequestURL();
const sessionId = useCookie('sessionId');

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
        Name: `${runtimeConfig.public.site.name} - File Uploader`,
        DestinationType: 'ImageUploader, TextUploader, FileUploader',
        RequestMethod: 'POST',
        RequestURL: `${requestUrl.origin}/api/files`,
        Headers: {
            Authorization: sessionId.value,
        },
        URL: '{json:url}',
        ErrorMessage: '{json:error}',
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
</script>
