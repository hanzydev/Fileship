<template>
    <UiCard v-if="canUseToken">
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

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('webhooks.title') }}</UiCardTitle>
            <UiCardDescription>{{ $t('webhooks.description') }}</UiCardDescription>
            <UiCardAction>
                <UiButton
                    :disabled="!webhooks || webhooks.length >= WEBHOOK_LIMIT"
                    @click="openWebhookSheet()"
                >
                    <IPlus />
                    {{ $t('webhooks.add') }}
                </UiButton>
            </UiCardAction>
        </UiCardHeader>
        <UiCardContent class="flex flex-col gap-2">
            <template v-if="!webhooks">
                <ListCardSkeleton v-for="i in 2" :key="i" media="size-4" />
            </template>
            <EmptyState
                v-else-if="!webhooks.length"
                :icon="IWebhook"
                :title="$t('webhooks.empty')"
                :description="$t('webhooks.emptyDescription')"
            />
            <UiCard v-for="webhook in webhooks" v-else :key="webhook.id" size="sm">
                <UiCardContent class="flex flex-col gap-3">
                    <div class="flex min-h-8 items-center gap-3">
                        <IWebhook class="size-4 shrink-0 text-muted-foreground" />
                        <div class="flex min-w-0 flex-1 flex-col gap-1">
                            <div class="flex min-w-0 items-center gap-2">
                                <TextBlock variant="small" class="truncate">
                                    {{ webhook.name }}
                                </TextBlock>
                                <UiBadge v-if="!webhook.enabled" variant="outline">
                                    {{ $t('webhooks.off') }}
                                </UiBadge>
                            </div>
                            <TextBlock variant="muted" class="truncate text-xs">
                                {{ WEBHOOK_FORMATS[webhook.format].label }} · {{ webhook.host }} ·
                                {{ $t('webhooks.eventCount', webhook.events.length) }}
                            </TextBlock>
                            <TextBlock
                                variant="muted"
                                :class="
                                    cn(
                                        'truncate text-xs',
                                        webhook.lastDelivery?.status ===
                                            WebhookDeliveryStatus.Failed && 'text-destructive',
                                    )
                                "
                            >
                                {{ lastDeliveryText(webhook) }}
                            </TextBlock>
                        </div>
                        <UiSwitch
                            :model-value="webhook.enabled"
                            :aria-label="$t('webhooks.toggle', { name: webhook.name })"
                            @update:model-value="handleToggleWebhook(webhook, $event)"
                        />
                        <UiDropdownMenu>
                            <UiDropdownMenuTrigger as-child>
                                <UiButton variant="ghost" size="icon-sm">
                                    <IEllipsis />
                                </UiButton>
                            </UiDropdownMenuTrigger>
                            <UiDropdownMenuContent align="end" class="w-56">
                                <UiDropdownMenuItem
                                    :disabled="testingId === webhook.id"
                                    @select="handleTestWebhook(webhook)"
                                >
                                    <ISend />
                                    {{ $t('webhooks.sendTest') }}
                                </UiDropdownMenuItem>
                                <UiDropdownMenuItem @select="openDeliveries(webhook)">
                                    <IHistory />
                                    {{ $t('webhooks.showDeliveries') }}
                                </UiDropdownMenuItem>
                                <UiDropdownMenuItem @select="openWebhookSheet(webhook)">
                                    <IPencil />
                                    {{ $t('webhooks.edit') }}
                                </UiDropdownMenuItem>
                                <UiDropdownMenuSeparator />
                                <UiDropdownMenuItem
                                    variant="destructive"
                                    @select="handleDeleteWebhook(webhook)"
                                >
                                    <ITrash2 />
                                    {{ $t('webhooks.delete') }}
                                </UiDropdownMenuItem>
                            </UiDropdownMenuContent>
                        </UiDropdownMenu>
                    </div>
                    <UiAlert v-if="webhook.disabledAt && !webhook.enabled" variant="destructive">
                        <ICircleAlert />
                        <UiAlertTitle>{{ $t('webhooks.disabledTitle') }}</UiAlertTitle>
                        <UiAlertDescription>
                            {{
                                $t('webhooks.disabledDescription', {
                                    count: WEBHOOK_FAILURE_LIMIT,
                                    time: date(webhook.disabledAt).fromNow(),
                                })
                            }}
                        </UiAlertDescription>
                    </UiAlert>
                </UiCardContent>
            </UiCard>
        </UiCardContent>
    </UiCard>

    <UiSheet v-model:open="webhookSheet.open">
        <UiSheetContent class="data-[side=right]:w-full data-[side=right]:sm:max-w-md">
            <form class="flex flex-1 flex-col overflow-hidden" @submit.prevent="handleSaveWebhook">
                <UiSheetHeader>
                    <UiSheetTitle>
                        {{ webhookSheet.id ? $t('webhooks.editTitle') : $t('webhooks.addTitle') }}
                    </UiSheetTitle>
                    <UiSheetDescription>{{ $t('webhooks.dialogDescription') }}</UiSheetDescription>
                </UiSheetHeader>

                <div class="flex flex-1 flex-col gap-6 overflow-y-auto px-6 pb-6">
                    <UiFieldGroup>
                        <UiField :data-invalid="!!webhookErrors?.name">
                            <UiFieldLabel for="webhook-name">
                                {{ $t('webhooks.name') }}
                            </UiFieldLabel>
                            <UiInput
                                id="webhook-name"
                                v-model="webhookForm.name"
                                maxlength="64"
                                autocomplete="off"
                                :placeholder="$t('webhooks.namePlaceholder')"
                            />
                            <UiFieldError
                                v-if="webhookErrors?.name"
                                :errors="webhookErrors.name._errors"
                            />
                        </UiField>

                        <UiField>
                            <UiFieldLabel>{{ $t('webhooks.format') }}</UiFieldLabel>
                            <UiTabs v-model="webhookForm.format">
                                <UiTabsList class="w-full">
                                    <UiTabsTrigger
                                        v-for="(format, key) in WEBHOOK_FORMATS"
                                        :key="key"
                                        :value="key"
                                    >
                                        {{ format.label }}
                                    </UiTabsTrigger>
                                </UiTabsList>
                            </UiTabs>
                            <UiFieldDescription>
                                {{ $t(WEBHOOK_FORMATS[webhookForm.format].description) }}
                                <a
                                    v-if="webhookForm.format === WebhookFormat.Json"
                                    :href="WEBHOOK_DOCS_URL"
                                    target="_blank"
                                    rel="noopener noreferrer"
                                >
                                    {{ $t('webhooks.docs') }}
                                </a>
                            </UiFieldDescription>
                        </UiField>

                        <UiField :data-invalid="!!webhookErrors?.url">
                            <UiFieldLabel for="webhook-url">{{ $t('webhooks.url') }}</UiFieldLabel>
                            <UiInput
                                id="webhook-url"
                                v-model="webhookForm.url"
                                type="url"
                                autocomplete="off"
                                :placeholder="
                                    webhookSheet.id
                                        ? $t('webhooks.keepCurrent')
                                        : URL_PLACEHOLDERS[webhookForm.format]
                                "
                            />
                            <UiFieldDescription>
                                {{ $t('webhooks.urlDescription') }}
                            </UiFieldDescription>
                            <UiFieldError
                                v-if="webhookErrors?.url"
                                :errors="webhookErrors.url._errors"
                            />
                        </UiField>

                        <UiField
                            v-if="webhookForm.format === WebhookFormat.Json"
                            :data-invalid="!!webhookErrors?.secret"
                        >
                            <UiFieldLabel for="webhook-secret">
                                {{ $t('webhooks.secret') }}
                            </UiFieldLabel>
                            <UiInputGroup>
                                <UiInputGroupInput
                                    id="webhook-secret"
                                    v-model="webhookForm.secret"
                                    autocomplete="off"
                                    class="font-mono"
                                    :placeholder="
                                        webhookSheet.hasSecret ? $t('webhooks.keepCurrent') : ''
                                    "
                                />
                                <UiInputGroupAddon align="inline-end">
                                    <UiInputGroupButton
                                        size="icon-xs"
                                        :aria-label="$t('webhooks.generate')"
                                        @click="handleGenerateSecret"
                                    >
                                        <IRefreshCw />
                                    </UiInputGroupButton>
                                    <UiInputGroupButton
                                        size="icon-xs"
                                        :aria-label="$t('webhooks.copySecret')"
                                        :disabled="!webhookForm.secret"
                                        @click="handleCopySecret"
                                    >
                                        <ICopy />
                                    </UiInputGroupButton>
                                </UiInputGroupAddon>
                            </UiInputGroup>
                            <UiFieldDescription>
                                {{ $t('webhooks.secretDescription') }}
                            </UiFieldDescription>
                            <UiFieldError
                                v-if="webhookErrors?.secret"
                                :errors="webhookErrors.secret._errors"
                            />
                        </UiField>
                    </UiFieldGroup>

                    <UiFieldSet>
                        <UiFieldLegend>{{ $t('webhooks.eventsTitle') }}</UiFieldLegend>
                        <UiFieldGroup class="gap-3">
                            <UiField
                                v-for="[key, event] in availableEvents"
                                :key="key"
                                orientation="horizontal"
                            >
                                <UiCheckbox
                                    :id="`webhook-event-${key}`"
                                    :model-value="webhookForm.events.includes(key)"
                                    @update:model-value="handleToggleEvent(key)"
                                />
                                <UiFieldContent>
                                    <UiFieldLabel :for="`webhook-event-${key}`">
                                        {{ $t(event.label) }}
                                    </UiFieldLabel>
                                    <UiFieldDescription>
                                        {{ $t(event.description) }}
                                    </UiFieldDescription>
                                </UiFieldContent>
                            </UiField>
                        </UiFieldGroup>
                        <UiFieldError
                            v-if="webhookErrors?.events"
                            :errors="webhookErrors.events._errors"
                        />
                    </UiFieldSet>

                    <UiAlert>
                        <IShieldCheck />
                        <UiAlertDescription>{{ $t('webhooks.privacy') }}</UiAlertDescription>
                    </UiAlert>
                </div>

                <UiSheetFooter>
                    <UiButton type="submit" :disabled="isSavingWebhook">
                        <UiSpinner v-if="isSavingWebhook" />
                        {{ $t('webhooks.save') }}
                    </UiButton>
                    <UiSheetClose as-child>
                        <UiButton type="button" variant="outline">
                            {{ $t('common.cancel') }}
                        </UiButton>
                    </UiSheetClose>
                </UiSheetFooter>
            </form>
        </UiSheetContent>
    </UiSheet>

    <UiSheet v-model:open="deliveriesSheet.open">
        <UiSheetContent class="data-[side=right]:w-full data-[side=right]:sm:max-w-md">
            <UiSheetHeader>
                <UiSheetTitle>{{ $t('webhooks.deliveries') }}</UiSheetTitle>
                <UiSheetDescription>
                    {{
                        $t('webhooks.deliveriesDescription', {
                            count: WEBHOOK_HISTORY,
                            host: deliveriesSheet.webhook?.host,
                        })
                    }}
                </UiSheetDescription>
            </UiSheetHeader>

            <div class="flex flex-1 flex-col gap-2 overflow-y-auto px-6 pb-6">
                <TextBlock v-if="deliveries?.pending" variant="muted" class="text-xs">
                    {{ $t('webhooks.pending', { count: deliveries.pending }) }}
                </TextBlock>
                <template v-if="!deliveries">
                    <ListCardSkeleton v-for="i in 3" :key="i" media="size-4" />
                </template>
                <EmptyState
                    v-else-if="!deliveries.items.length"
                    :icon="IHistory"
                    :title="$t('webhooks.neverDelivered')"
                    :description="$t('webhooks.noDeliveriesDescription')"
                />
                <UiItem
                    v-for="delivery in deliveries.items"
                    v-else
                    :key="delivery.id"
                    variant="outline"
                    size="sm"
                >
                    <UiItemContent class="min-w-0">
                        <UiItemTitle class="w-full">
                            <UiBadge :variant="STATUS_VARIANTS[delivery.status]">
                                {{ $t(`webhooks.status.${delivery.status}`) }}
                            </UiBadge>
                            <span class="truncate">{{ $t(eventLabel(delivery.event)) }}</span>
                        </UiItemTitle>
                        <UiItemDescription>
                            {{ date(delivery.createdAt).fromNow() }}
                            <template v-if="delivery.error || delivery.statusCode">
                                · {{ deliveryResult(delivery) }}
                            </template>
                            <template v-if="delivery.duration !== null">
                                · {{ delivery.duration }} ms
                            </template>
                            <template v-if="delivery.attempts">
                                · {{ $t('webhooks.attempts', delivery.attempts) }}
                            </template>
                        </UiItemDescription>
                        <UiItemDescription v-if="delivery.status === WebhookDeliveryStatus.Pending">
                            {{
                                $t('webhooks.nextAttempt', {
                                    time: date(delivery.nextAttemptAt).fromNow(),
                                })
                            }}
                        </UiItemDescription>
                        <code
                            v-if="delivery.detail"
                            class="line-clamp-2 rounded-lg bg-muted px-2 py-1 font-mono text-xs break-all text-muted-foreground"
                        >
                            {{ delivery.detail }}
                        </code>
                    </UiItemContent>
                </UiItem>
            </div>

            <UiSheetFooter>
                <UiButton
                    :disabled="testingId === deliveriesSheet.webhook?.id"
                    @click="handleTestWebhook(deliveriesSheet.webhook!)"
                >
                    <UiSpinner v-if="testingId === deliveriesSheet.webhook?.id" />
                    <ISend v-else />
                    {{ $t('webhooks.sendTest') }}
                </UiButton>
                <UiSheetClose as-child>
                    <UiButton type="button" variant="outline">
                        {{ $t('common.close') }}
                    </UiButton>
                </UiSheetClose>
            </UiSheetFooter>
        </UiSheetContent>
    </UiSheet>
</template>

<script setup lang="ts">
import { ICircleAlert, IHistory, IWebhook } from '#components';
import { WebhookDeliveryStatus, WebhookFormat } from '#shared/prisma/enums';

import type { WebhookData, WebhookDeliveryData } from '~/types/webhooks';

const publicSettings = usePublicSettings();
const authUser = useAuthUser();
const requestUrl = useRequestURL();
const webhooks = useWebhooks();
const { $toast } = useNuxtApp();
const { t } = useI18n();
const { copy } = useClipboard({ legacy: true });
const date = useDate();

const canUpload = computed(() => canUploadFiles(authUser.value));
const canShorten = computed(() => canShortenLinks(authUser.value));
const canNote = computed(() => canTakeNotes(authUser.value));
const canUseToken = computed(() => canUpload.value || canShorten.value);

const { data: uploadToken } = useFetch('/api/users/@me/upload-token', {
    server: false,
    immediate: canUseToken.value,
});

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

const URL_PLACEHOLDERS = {
    [WebhookFormat.Discord]: 'https://discord.com/api/webhooks/…',
    [WebhookFormat.Slack]: 'https://hooks.slack.com/services/…',
    [WebhookFormat.Json]: 'https://example.com/fileship',
};

const STATUS_VARIANTS = {
    [WebhookDeliveryStatus.Pending]: 'secondary',
    [WebhookDeliveryStatus.Processing]: 'secondary',
    [WebhookDeliveryStatus.Succeeded]: 'default',
    [WebhookDeliveryStatus.Failed]: 'destructive',
} as const;

const DEFAULT_EVENTS = ['file.uploaded', 'link.created', 'note.created'];

const webhookSheet = reactive({ open: false, id: null as string | null, hasSecret: false });
const webhookForm = reactive({
    name: '',
    url: '',
    format: WebhookFormat.Discord as WebhookFormat,
    secret: '',
    events: [] as string[],
});
const webhookErrors = ref<any>();
const isSavingWebhook = ref(false);
const testingId = ref<string | null>(null);

const deliveriesSheet = reactive({ open: false, webhook: null as WebhookData | null });
const deliveries = ref<{ pending: number; items: WebhookDeliveryData[] } | null>(null);

const availableEvents = computed(() => {
    const allowed = { files: canUpload.value, links: canShorten.value, notes: canNote.value };

    return (
        Object.entries(WEBHOOK_EVENTS) as [WebhookEvent, (typeof WEBHOOK_EVENTS)[WebhookEvent]][]
    ).filter(([, event]) => allowed[event.permission]);
});

const randomSecret = () =>
    `whsec_${Array.from(crypto.getRandomValues(new Uint8Array(24)), (byte) =>
        byte.toString(16).padStart(2, '0'),
    ).join('')}`;

const eventLabel = (event: string) =>
    WEBHOOK_EVENTS[event as WebhookEvent]?.label ?? 'webhooks.events.test';

const deliveryResult = (delivery: WebhookDeliveryData) =>
    !delivery.error || delivery.error === 'status'
        ? t('webhooks.errors.status', { code: delivery.statusCode })
        : t(`webhooks.errors.${delivery.error}`);

const lastDeliveryText = (webhook: WebhookData) => {
    const delivery = webhook.lastDelivery;
    if (!delivery) return t('webhooks.neverDelivered');

    return t(
        delivery.status === WebhookDeliveryStatus.Succeeded
            ? 'webhooks.deliveredAgo'
            : 'webhooks.failedAgo',
        { time: date(delivery.finishedAt!).fromNow() },
    );
};

const upsertWebhook = (webhook: WebhookData) => {
    webhooks.value = webhooks.value!.some((item) => item.id === webhook.id)
        ? webhooks.value!.map((item) => (item.id === webhook.id ? webhook : item))
        : [...webhooks.value!, webhook];
};

const openWebhookSheet = (webhook?: WebhookData) => {
    webhookErrors.value = undefined;
    webhookSheet.id = webhook?.id ?? null;
    webhookSheet.hasSecret = !!webhook?.hasSecret;

    Object.assign(webhookForm, {
        name: webhook?.name ?? '',
        url: '',
        format: webhook?.format ?? WebhookFormat.Discord,
        secret: webhook?.hasSecret ? '' : randomSecret(),
        events: webhook
            ? [...webhook.events]
            : DEFAULT_EVENTS.filter((event) =>
                  availableEvents.value.some(([key]) => key === event),
              ),
    });

    webhookSheet.open = true;
};

const handleToggleEvent = (event: string) => {
    webhookForm.events = webhookForm.events.includes(event)
        ? webhookForm.events.filter((item) => item !== event)
        : [...webhookForm.events, event];
};

const handleGenerateSecret = () => {
    webhookForm.secret = randomSecret();
};

const handleCopySecret = () => {
    copy(webhookForm.secret);
    $toast.success(t('webhooks.secretCopied'));
};

const handleSaveWebhook = async () => {
    isSavingWebhook.value = true;
    webhookErrors.value = undefined;

    const body = {
        name: webhookForm.name,
        url: webhookForm.url,
        format: webhookForm.format,
        events: webhookForm.events,
        secret: webhookForm.format === WebhookFormat.Json ? webhookForm.secret : '',
    };

    try {
        upsertWebhook(
            webhookSheet.id
                ? await $fetch<WebhookData>(`/api/users/@me/webhooks/${webhookSheet.id}`, {
                      method: 'PATCH',
                      body,
                  })
                : await $fetch<WebhookData>('/api/users/@me/webhooks', {
                      method: 'POST',
                      body: { ...body, enabled: true },
                  }),
        );

        $toast.success(t(webhookSheet.id ? 'webhooks.saved' : 'webhooks.created'));
        webhookSheet.open = false;
    } catch (error: any) {
        webhookErrors.value = error?.data?.data?.formErrors;
        if (!webhookErrors.value) $toast.error(getErrorMessage(error));
    } finally {
        isSavingWebhook.value = false;
    }
};

const handleToggleWebhook = async (webhook: WebhookData, enabled: boolean) => {
    try {
        upsertWebhook(
            await $fetch<WebhookData>(`/api/users/@me/webhooks/${webhook.id}`, {
                method: 'PATCH',
                body: { enabled },
            }),
        );

        $toast.success(t(enabled ? 'webhooks.turnedOn' : 'webhooks.turnedOff'));
    } catch (error) {
        $toast.error(getErrorMessage(error));
    }
};

const handleTestWebhook = async (webhook: WebhookData) => {
    testingId.value = webhook.id;

    try {
        const delivery = await $fetch<WebhookDeliveryData>(
            `/api/users/@me/webhooks/${webhook.id}/test`,
            { method: 'POST' },
        );

        if (delivery.status === WebhookDeliveryStatus.Succeeded) {
            $toast.success(t('webhooks.testDelivered'));
        } else {
            $toast.error(t('webhooks.testFailed', { error: deliveryResult(delivery) }), {
                description: delivery.detail ?? undefined,
            });
        }
    } catch (error) {
        $toast.error(getErrorMessage(error));
    } finally {
        testingId.value = null;
    }
};

const handleDeleteWebhook = (webhook: WebhookData) =>
    askConfirmation({
        title: t('webhooks.deleteTitle'),
        description: t('webhooks.deleteDescription', { host: webhook.host }),
        confirmText: t('webhooks.delete'),
        destructive: true,
        action: async () => {
            try {
                await $fetch(`/api/users/@me/webhooks/${webhook.id}`, { method: 'DELETE' });

                webhooks.value = webhooks.value!.filter((item) => item.id !== webhook.id);
                $toast.success(t('webhooks.deleted'));
            } catch (error) {
                $toast.error(getErrorMessage(error));
            }
        },
    });

const loadDeliveries = async () => {
    const webhookId = deliveriesSheet.webhook!.id;
    const result = await $fetch<{ pending: number; items: WebhookDeliveryData[] }>(
        `/api/users/@me/webhooks/${webhookId}/deliveries`,
    ).catch(() => null);

    if (result && deliveriesSheet.webhook?.id === webhookId) deliveries.value = result;
};

const openDeliveries = (webhook: WebhookData) => {
    deliveriesSheet.webhook = webhook;
    deliveries.value = null;
    deliveriesSheet.open = true;
    loadDeliveries();
};

watch(
    () => webhooks.value?.find((webhook) => webhook.id === deliveriesSheet.webhook?.id),
    (webhook) => {
        if (!webhook || !deliveriesSheet.open) return;

        deliveriesSheet.webhook = webhook;
        loadDeliveries();
    },
);

onMounted(async () => {
    webhooks.value = await $fetch<WebhookData[]>('/api/users/@me/webhooks').catch(() => []);
});

definePageMeta({
    middleware: [
        () => {
            const user = useAuthUser();

            if (!canUploadFiles(user) && !canShortenLinks(user) && !canTakeNotes(user)) {
                return abortNavigation({
                    status: 403,
                    message: useNuxtApp().$i18n.t('auth.noPermission'),
                });
            }
        },
    ],
});
</script>
