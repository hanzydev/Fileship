<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('general.startup') }}</UiCardTitle>
            <UiCardDescription>
                {{ $t(isLinux ? 'general.startupDescriptionLinux' : 'general.startupDescription') }}
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiField orientation="horizontal">
                <UiFieldLabel for="start-at-login">
                    {{ $t(isLinux ? 'general.startAtLoginLinux' : 'general.startAtLogin') }}
                </UiFieldLabel>
                <UiSwitch
                    id="start-at-login"
                    :model-value="settings?.startAtLogin"
                    :disabled="!settings"
                    @update:model-value="updateSettings({ startAtLogin: $event })"
                />
            </UiField>
        </UiCardContent>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('general.language') }}</UiCardTitle>
            <UiCardDescription>{{ $t('general.languageDescription') }}</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiSelect
                :model-value="settings?.language"
                :disabled="!settings"
                @update:model-value="updateSettings({ language: $event as Settings['language'] })"
            >
                <UiSelectTrigger class="w-full sm:w-64">
                    <UiSelectValue />
                </UiSelectTrigger>
                <UiSelectContent>
                    <UiSelectItem
                        v-for="language in ['system', ...LANGUAGES]"
                        :key="language"
                        :value="language"
                    >
                        {{
                            $t(
                                language === 'system' && isLinux
                                    ? 'labels.language.systemLinux'
                                    : `labels.language.${language}`,
                            )
                        }}
                    </UiSelectItem>
                </UiSelectContent>
            </UiSelect>
        </UiCardContent>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('general.theme') }}</UiCardTitle>
            <UiCardDescription>
                {{ $t(isLinux ? 'general.themeDescriptionLinux' : 'general.themeDescription') }}
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiTabs v-model="colorMode.preference">
                <UiTabsList class="w-full">
                    <UiTabsTrigger value="light">
                        <ISun />
                        {{ $t('general.light') }}
                    </UiTabsTrigger>
                    <UiTabsTrigger value="dark">
                        <IMoon />
                        {{ $t('general.dark') }}
                    </UiTabsTrigger>
                    <UiTabsTrigger value="system">
                        <IMonitor />
                        {{ $t('general.system') }}
                    </UiTabsTrigger>
                </UiTabsList>
            </UiTabs>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
import { LANGUAGES } from '../../../i18n/messages';
import type { Settings } from '../../../shared/settings';

const appInfo = useAppInfo();
const colorMode = useColorMode();
const settings = useSettings();

const isLinux = computed(() => appInfo.value.platform === 'linux');
</script>
