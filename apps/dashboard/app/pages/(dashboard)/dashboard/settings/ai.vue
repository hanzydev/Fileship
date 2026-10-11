<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>AI</UiCardTitle>
            <UiCardDescription>
                Captions, smart search and sensitive content detection for your files.
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiFieldGroup>
                <UiField orientation="horizontal">
                    <UiFieldContent>
                        <UiFieldLabel for="ai-enabled">AI features</UiFieldLabel>
                        <UiFieldDescription>
                            Analyze uploads to enable captions and smart search.
                        </UiFieldDescription>
                    </UiFieldContent>
                    <UiSwitch
                        id="ai-enabled"
                        :model-value="currentUser!.aiSettings.enabled"
                        @update:model-value="handleUpdate({ enabled: $event })"
                    />
                </UiField>
                <UiFieldSeparator />
                <UiField orientation="horizontal">
                    <UiFieldContent>
                        <UiFieldLabel for="ai-pii">Hide sensitive previews</UiFieldLabel>
                        <UiFieldDescription>
                            Blur image previews that may contain personal information.
                        </UiFieldDescription>
                    </UiFieldContent>
                    <UiSwitch
                        id="ai-pii"
                        :model-value="currentUser!.aiSettings.suppressPii"
                        @update:model-value="handleUpdate({ suppressPii: $event })"
                    />
                </UiField>
            </UiFieldGroup>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
const currentUser = useAuthUser();
const { $toast } = useNuxtApp();

const handleUpdate = async (body: { enabled?: boolean; suppressPii?: boolean }) => {
    const previous = { ...currentUser.value!.aiSettings };
    Object.assign(currentUser.value!.aiSettings, body);

    try {
        await $fetch('/api/users/@me/ai-settings', { method: 'PATCH', body });
        $toast.success('AI settings saved');
    } catch (error: any) {
        currentUser.value!.aiSettings = previous;
        $toast.error(error.data.message);
    }
};
</script>
