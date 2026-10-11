<template>
    <UiCard>
        <form class="contents" @submit.prevent="handleSaveEmbed">
            <UiCardHeader>
                <UiCardTitle>Embeds</UiCardTitle>
                <UiCardDescription>
                    How your links look when shared on Discord, Twitter and other platforms.
                </UiCardDescription>
                <UiCardAction>
                    <UiSwitch v-model="embedForm.enabled" aria-label="Enable embeds" />
                </UiCardAction>
            </UiCardHeader>
            <UiCardContent class="grid gap-6 md:grid-cols-2">
                <UiFieldGroup>
                    <UiField>
                        <UiFieldLabel for="embed-title">Title</UiFieldLabel>
                        <UiInput id="embed-title" v-model="embedForm.title" />
                    </UiField>
                    <UiField>
                        <UiFieldLabel for="embed-description">Description</UiFieldLabel>
                        <UiTextarea id="embed-description" v-model="embedForm.description" />
                    </UiField>
                    <UiField>
                        <UiFieldLabel for="embed-site">Site name</UiFieldLabel>
                        <UiInput id="embed-site" v-model="embedForm.siteName" />
                    </UiField>
                    <UiField>
                        <UiFieldLabel for="embed-color">Color</UiFieldLabel>
                        <UiInputGroup>
                            <UiInputGroupAddon>
                                <input
                                    v-model="embedForm.color"
                                    type="color"
                                    aria-label="Pick a color"
                                    class="size-5 rounded-full border-0 bg-transparent p-0"
                                />
                            </UiInputGroupAddon>
                            <UiInputGroupInput id="embed-color" v-model="embedForm.color" />
                        </UiInputGroup>
                    </UiField>
                    <UiFieldDescription>
                        You can use {fileName}, {mimeType}, {size}, {createdAt} and {now}.
                    </UiFieldDescription>
                </UiFieldGroup>

                <div class="flex flex-col gap-2">
                    <TextBlock variant="small">Preview</TextBlock>
                    <div
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
                                {{ fillPlaceholders(embedForm.title) || 'Untitled' }}
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
                    Save
                </UiButton>
            </UiCardFooter>
        </form>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>Domains</UiCardTitle>
            <UiCardDescription>
                Custom domains used when building links to your files. One is picked at random.
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent class="flex flex-col gap-3">
            <form class="flex gap-2" @submit.prevent="handleAddDomain">
                <UiInput v-model="newDomain" placeholder="i.example.com" />
                <UiButton type="submit" variant="outline" :disabled="!newDomain.trim()">
                    <IPlus />
                    Add
                </UiButton>
            </form>
            <UiCard v-for="domain in domainsForm" :key="domain" size="sm">
                <UiCardContent class="flex items-center gap-3">
                    <IGlobe class="size-4 text-muted-foreground" />
                    <span class="flex-1 truncate text-sm">{{ domain }}</span>
                    <UiButton
                        variant="ghost"
                        size="icon-sm"
                        @click="domainsForm = domainsForm.filter((d) => d !== domain)"
                    >
                        <IX />
                    </UiButton>
                </UiCardContent>
            </UiCard>
            <EmptyState
                v-if="!domainsForm.length"
                :icon="IGlobe"
                title="No custom domains"
                description="Links use the current domain."
            />
        </UiCardContent>
        <UiCardFooter class="justify-end">
            <UiButton :disabled="isSavingDomains || !isDomainsDirty" @click="handleSaveDomains">
                <UiSpinner v-if="isSavingDomains" />
                Save
            </UiButton>
        </UiCardFooter>
    </UiCard>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

import { IGlobe } from '#components';

const embed = useEmbed();
const domains = useDomains();
const { $toast } = useNuxtApp();

const embedForm = reactive({ ...embed.value });
const domainsForm = ref([...domains.value]);
const newDomain = ref('');
const isSavingEmbed = ref(false);
const isSavingDomains = ref(false);

const isEmbedDirty = computed(() => JSON.stringify(embedForm) !== JSON.stringify(embed.value));
const isDomainsDirty = computed(() => domainsForm.value.join() !== domains.value.join());

const fillPlaceholders = (text: string) =>
    text
        .replaceAll('{fileName}', 'holiday.png')
        .replaceAll('{mimeType}', 'image/png')
        .replaceAll('{size}', '2.4 MB')
        .replaceAll('{createdAt}', dayjs().format('MMM D, YYYY'))
        .replaceAll('{now}', dayjs().format('MMM D, YYYY HH:mm'));

const handleSaveEmbed = async () => {
    isSavingEmbed.value = true;

    try {
        await $fetch('/api/users/@me/embed', { method: 'PATCH', body: embedForm });
        $toast.success('Embed settings saved');
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
        $toast.success('Domains saved');
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    isSavingDomains.value = false;
};

watch(embed, (value) => Object.assign(embedForm, value));
watch(domains, (value) => (domainsForm.value = [...value]));
</script>
