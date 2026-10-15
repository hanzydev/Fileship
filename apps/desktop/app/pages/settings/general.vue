<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>Startup</UiCardTitle>
            <UiCardDescription>
                Fileship starts in the tray, so your hotkeys work right after you sign in to
                Windows.
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiField orientation="horizontal">
                <UiFieldLabel for="start-at-login">Start Fileship with Windows</UiFieldLabel>
                <UiSwitch
                    id="start-at-login"
                    :model-value="settings?.startAtLogin"
                    :disabled="!settings"
                    @update:model-value="handleUpdate({ startAtLogin: $event })"
                />
            </UiField>
        </UiCardContent>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>Theme</UiCardTitle>
            <UiCardDescription>Light, dark or follow Windows.</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiTabs v-model="colorMode.preference">
                <UiTabsList class="w-full">
                    <UiTabsTrigger value="light">
                        <ISun />
                        Light
                    </UiTabsTrigger>
                    <UiTabsTrigger value="dark">
                        <IMoon />
                        Dark
                    </UiTabsTrigger>
                    <UiTabsTrigger value="system">
                        <IMonitor />
                        System
                    </UiTabsTrigger>
                </UiTabsList>
            </UiTabs>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
import type { Settings } from '../../../shared/ipc';

const colorMode = useColorMode();
const { $toast } = useNuxtApp();

const settings = ref<Settings | null>(null);

const handleUpdate = async (patch: Partial<Settings>) => {
    try {
        settings.value = await desktop('settings:set', patch);
    } catch (error: any) {
        $toast.error(error.message);
    }
};

onMounted(async () => {
    settings.value = await desktop('settings:get');
});
</script>
