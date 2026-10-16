<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('settings.ai.title') }}</UiCardTitle>
            <UiCardDescription>
                {{ $t('settings.ai.description') }}
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiFieldGroup>
                <UiField orientation="horizontal">
                    <UiFieldContent>
                        <UiFieldLabel for="ai-enabled">
                            {{ $t('settings.ai.enabled') }}
                        </UiFieldLabel>
                        <UiFieldDescription>
                            {{ $t('settings.ai.enabledDescription') }}
                        </UiFieldDescription>
                    </UiFieldContent>
                    <UiSwitch
                        id="ai-enabled"
                        :model-value="currentUser!.aiSettings.enabled"
                        :disabled="isUpdating"
                        @update:model-value="handleUpdate({ enabled: $event })"
                    />
                </UiField>
                <UiFieldSeparator />
                <UiField orientation="horizontal">
                    <UiFieldContent>
                        <UiFieldLabel for="ai-pii">
                            {{ $t('settings.ai.suppressPii') }}
                        </UiFieldLabel>
                        <UiFieldDescription>
                            {{ $t('settings.ai.suppressPiiDescription') }}
                        </UiFieldDescription>
                    </UiFieldContent>
                    <UiSwitch
                        id="ai-pii"
                        :model-value="currentUser!.aiSettings.suppressPii"
                        :disabled="isUpdating"
                        @update:model-value="handleUpdate({ suppressPii: $event })"
                    />
                </UiField>
            </UiFieldGroup>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
const { t } = useI18n();
const currentUser = useAuthUser();
const { $toast } = useNuxtApp();

const isUpdating = ref(false);

const handleUpdate = async (body: { enabled?: boolean; suppressPii?: boolean }) => {
    const previous = { ...currentUser.value!.aiSettings };
    Object.assign(currentUser.value!.aiSettings, body);
    isUpdating.value = true;

    try {
        await $fetch('/api/users/@me/ai-settings', { method: 'PATCH', body });
        $toast.success(t('settings.ai.saved'));
    } catch (error: any) {
        currentUser.value!.aiSettings = previous;
        $toast.error(getErrorMessage(error));
    }

    isUpdating.value = false;
};

definePageMeta({
    middleware: 'file-uploader-only',
});
</script>
