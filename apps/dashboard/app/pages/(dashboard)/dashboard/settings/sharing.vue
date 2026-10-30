<template>
    <UiCard>
        <form class="contents" @submit.prevent="handleSaveEmbed">
            <UiCardHeader>
                <UiCardTitle>{{ $t('sharing.embeds') }}</UiCardTitle>
                <UiCardDescription>{{ $t('sharing.embedsDescription') }}</UiCardDescription>
                <UiCardAction>
                    <UiSwitch
                        v-model="embedForm.enabled"
                        :aria-label="$t('sharing.enableEmbeds')"
                    />
                </UiCardAction>
            </UiCardHeader>
            <UiCardContent class="grid gap-6 md:grid-cols-2">
                <UiFieldGroup>
                    <UiField>
                        <UiFieldLabel for="embed-title">{{ $t('sharing.title') }}</UiFieldLabel>
                        <UiInput id="embed-title" v-model="embedForm.title" />
                    </UiField>
                    <UiField>
                        <UiFieldLabel for="embed-description">
                            {{ $t('sharing.description') }}
                        </UiFieldLabel>
                        <UiTextarea id="embed-description" v-model="embedForm.description" />
                    </UiField>
                    <UiField>
                        <UiFieldLabel for="embed-site">{{ $t('sharing.siteName') }}</UiFieldLabel>
                        <UiInput id="embed-site" v-model="embedForm.siteName" />
                    </UiField>
                    <UiField>
                        <UiFieldLabel for="embed-color">{{ $t('sharing.color') }}</UiFieldLabel>
                        <UiInputGroup>
                            <UiInputGroupAddon>
                                <input
                                    v-model="embedForm.color"
                                    type="color"
                                    :aria-label="$t('sharing.pickColor')"
                                    class="size-5 rounded-full border-0 bg-transparent p-0"
                                />
                            </UiInputGroupAddon>
                            <UiInputGroupInput id="embed-color" v-model="embedForm.color" />
                        </UiInputGroup>
                    </UiField>
                    <UiFieldDescription>
                        {{ $t('sharing.placeholders', PLACEHOLDERS) }}
                    </UiFieldDescription>
                    <UiSeparator />
                    <UiField
                        orientation="horizontal"
                        class="has-[>[data-slot=field-content]]:items-center"
                    >
                        <UiFieldContent>
                            <UiFieldLabel for="embed-discord">
                                {{ $t('sharing.discord.title') }}
                            </UiFieldLabel>
                            <UiFieldDescription>
                                {{ $t('sharing.discord.description') }}
                            </UiFieldDescription>
                        </UiFieldContent>
                        <UiSwitch
                            id="embed-discord"
                            v-model="embedForm.discord.enabled"
                            :disabled="!embedForm.enabled"
                        />
                    </UiField>
                    <UiButton
                        type="button"
                        variant="outline"
                        class="self-start"
                        :disabled="!isDiscordActive"
                        @click="isLayoutsOpen = true"
                    >
                        <ILayoutTemplate />
                        {{ $t('sharing.discord.customize') }}
                    </UiButton>
                </UiFieldGroup>

                <div class="flex flex-col gap-2">
                    <TextBlock variant="small">{{ $t('sharing.preview') }}</TextBlock>
                    <DiscordEmbedPreview
                        v-if="isDiscordActive"
                        :embed="preview('file', embedForm)"
                        :color="embedForm.color"
                    />
                    <div
                        v-else
                        :class="
                            cn(
                                'flex gap-3 rounded-xl border-l-4 bg-muted p-4 transition-opacity',
                                !embedForm.enabled && 'opacity-50',
                            )
                        "
                        :style="{ borderColor: embedForm.color }"
                    >
                        <div class="flex min-w-0 flex-1 flex-col gap-2">
                            <span v-if="embedForm.siteName" class="text-xs text-muted-foreground">
                                {{ fillPlaceholders(embedForm.siteName) }}
                            </span>
                            <span class="font-semibold break-words text-primary">
                                {{ fillPlaceholders(embedForm.title) || $t('sharing.untitled') }}
                            </span>
                            <span
                                v-if="embedForm.description"
                                class="text-sm break-words whitespace-pre-wrap"
                            >
                                {{ fillPlaceholders(embedForm.description) }}
                            </span>
                            <div
                                class="flex aspect-video items-center justify-center rounded-lg bg-background"
                            >
                                <IImage class="size-8 text-muted-foreground" />
                            </div>
                        </div>
                    </div>
                </div>
            </UiCardContent>
            <UiCardFooter class="justify-end">
                <UiButton type="submit" :disabled="isSavingEmbed || !isEmbedDirty">
                    <UiSpinner v-if="isSavingEmbed" />
                    {{ $t('sharing.save') }}
                </UiButton>
            </UiCardFooter>
        </form>
    </UiCard>

    <DiscordLayoutDialog
        v-model:open="isLayoutsOpen"
        v-model:discord="embedForm.discord"
        :embed="embedForm"
    />

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('sharing.domains') }}</UiCardTitle>
            <UiCardDescription>{{ $t('sharing.domainsDescription') }}</UiCardDescription>
        </UiCardHeader>
        <UiCardContent class="flex flex-col gap-3">
            <form class="flex gap-2" @submit.prevent="handleAddDomain">
                <UiInput v-model="newDomain" :placeholder="$t('sharing.domainPlaceholder')" />
                <UiButton type="submit" variant="outline" :disabled="!newDomain.trim()">
                    <IPlus />
                    {{ $t('sharing.add') }}
                </UiButton>
            </form>
            <UiCard v-for="domain in domainsForm" :key="domain" size="sm">
                <UiCardContent class="flex items-center gap-3">
                    <IGlobe class="size-4 text-muted-foreground" />
                    <span class="flex-1 truncate text-sm">{{ domain }}</span>
                    <UiButton
                        variant="ghost"
                        size="icon-sm"
                        :aria-label="$t('sharing.removeDomain', { domain })"
                        @click="domainsForm = domainsForm.filter((d) => d !== domain)"
                    >
                        <IX />
                    </UiButton>
                </UiCardContent>
            </UiCard>
            <EmptyState
                v-if="!domainsForm.length"
                :icon="IGlobe"
                :title="$t('sharing.noDomains')"
                :description="$t('sharing.noDomainsDescription')"
            />
        </UiCardContent>
        <UiCardFooter class="justify-end">
            <UiButton :disabled="isSavingDomains || !isDomainsDirty" @click="handleSaveDomains">
                <UiSpinner v-if="isSavingDomains" />
                {{ $t('sharing.save') }}
            </UiButton>
        </UiCardFooter>
    </UiCard>
</template>

<script setup lang="ts">
import { filesize } from 'filesize';

import { IGlobe } from '#components';

const embed = useEmbed();
const domains = useDomains();
const { $toast } = useNuxtApp();
const { t, locale } = useI18n();
const date = useDate();

const PLACEHOLDERS = {
    fileName: '{fileName}',
    mimeType: '{mimeType}',
    size: '{size}',
    createdAt: '{createdAt}',
    now: '{now}',
};

const { preview } = useDiscordPreview();

const copyEmbed = (value: IEmbed): IEmbed => JSON.parse(JSON.stringify(value));

const embedForm = reactive(copyEmbed(embed.value));
const domainsForm = ref([...domains.value]);
const newDomain = ref('');
const isSavingEmbed = ref(false);
const isSavingDomains = ref(false);
const isLayoutsOpen = ref(false);

const isDiscordActive = computed(() => embedForm.enabled && embedForm.discord.enabled);

const isEmbedDirty = computed(() => JSON.stringify(embedForm) !== JSON.stringify(embed.value));
const isDomainsDirty = computed(() => domainsForm.value.join() !== domains.value.join());

const fillPlaceholders = (text: string) =>
    text
        .replaceAll('{fileName}', 'holiday.png')
        .replaceAll('{mimeType}', 'image/png')
        .replaceAll('{size}', filesize(2_400_000, { locale: locale.value }))
        .replaceAll('{createdAt}', date().format('ll'))
        .replaceAll('{now}', date().format('lll'));

const handleSaveEmbed = async () => {
    isSavingEmbed.value = true;

    try {
        await $fetch('/api/users/@me/embed', { method: 'PATCH', body: embedForm });
        $toast.success(t('sharing.embedSaved'));
    } catch (error: any) {
        $toast.error(error.data?.data?.formErrors?.color?._errors[0] || getErrorMessage(error));
    }

    isSavingEmbed.value = false;
};

const handleAddDomain = () => {
    const domain = newDomain.value
        .trim()
        .replace(/^https?:\/\//, '')
        .replace(/\/.*$/, '');

    if (!domainsForm.value.includes(domain)) domainsForm.value.push(domain);
    newDomain.value = '';
};

const handleSaveDomains = async () => {
    isSavingDomains.value = true;

    try {
        await $fetch('/api/users/@me/domains', { method: 'PUT', body: domainsForm.value });
        $toast.success(t('sharing.domainsSaved'));
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    isSavingDomains.value = false;
};

watch(embed, (value) => Object.assign(embedForm, copyEmbed(value)));
watch(domains, (value) => (domainsForm.value = [...value]));
</script>
