<template>
    <Head>
        <Title>{{ $t('shareTarget.title') }}</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader :title="$t('shareTarget.title')" :description="$t('shareTarget.description')" />

        <UiAlert v-if="receiveFailed" variant="destructive" class="mx-auto max-w-2xl">
            <ICircleAlert />
            <UiAlertTitle>{{ $t('shareTarget.receiveFailed') }}</UiAlertTitle>
            <UiAlertDescription>
                {{ $t('shareTarget.receiveFailedDescription') }}
            </UiAlertDescription>
        </UiAlert>

        <div v-if="isLoading" class="flex flex-1 items-center justify-center">
            <UiSpinner class="size-6" />
        </div>

        <EmptyState
            v-else-if="!hasPending && !results.length"
            :icon="IShare2"
            :title="$t('shareTarget.empty')"
            :description="$t('shareTarget.emptyDescription')"
        >
            <UiButton variant="outline" as-child>
                <NuxtLink to="/dashboard">{{ $t('shareTarget.goToDashboard') }}</NuxtLink>
            </UiButton>
        </EmptyState>

        <div v-else class="mx-auto flex w-full max-w-2xl flex-col gap-4">
            <UiCard v-if="results.length">
                <UiCardHeader>
                    <UiCardTitle>{{ $t('shareTarget.results') }}</UiCardTitle>
                    <UiCardDescription>
                        {{ $t('shareTarget.resultsDescription') }}
                    </UiCardDescription>
                    <UiCardAction v-if="results.length > 1">
                        <UiButton variant="outline" size="sm" @click="handleCopyAll">
                            <ICopy />
                            {{ $t('shareTarget.copyAll') }}
                        </UiButton>
                    </UiCardAction>
                </UiCardHeader>
                <UiCardContent class="flex flex-col gap-2">
                    <UiItem v-for="result in results" :key="result.url" variant="muted" size="sm">
                        <UiItemMedia variant="icon">
                            <component :is="resultIcons[result.type]" />
                        </UiItemMedia>
                        <UiItemContent class="min-w-0">
                            <UiItemTitle class="max-w-full">
                                <span class="truncate">{{ result.name }}</span>
                            </UiItemTitle>
                            <UiItemDescription class="line-clamp-1 break-all">
                                {{ result.url }}
                            </UiItemDescription>
                        </UiItemContent>
                        <UiItemActions>
                            <UiButton
                                variant="ghost"
                                size="icon-sm"
                                :aria-label="$t('shareTarget.copy')"
                                @click="handleCopy(result.url)"
                            >
                                <ICopy />
                            </UiButton>
                            <UiButton variant="ghost" size="icon-sm" as-child>
                                <a
                                    :href="result.url"
                                    target="_blank"
                                    rel="noopener noreferrer"
                                    :aria-label="$t('shareTarget.open')"
                                >
                                    <IExternalLink />
                                </a>
                            </UiButton>
                        </UiItemActions>
                    </UiItem>
                </UiCardContent>
                <UiCardFooter v-if="!hasPending">
                    <UiButton class="w-full sm:w-auto" as-child>
                        <NuxtLink to="/dashboard">{{ $t('shareTarget.done') }}</NuxtLink>
                    </UiButton>
                </UiCardFooter>
            </UiCard>

            <UiCard v-if="files.length">
                <UiCardHeader>
                    <UiCardTitle>{{ $t('shareTarget.files', files.length) }}</UiCardTitle>
                    <UiCardDescription>
                        {{
                            canUpload
                                ? $t('shareTarget.filesDescription')
                                : $t('shareTarget.cannotUpload')
                        }}
                    </UiCardDescription>
                </UiCardHeader>
                <UiCardContent class="flex flex-col gap-6">
                    <div class="flex flex-col gap-2">
                        <UiItem v-for="item in files" :key="item.key" variant="muted" size="sm">
                            <UiItemMedia variant="icon">
                                <IFile />
                            </UiItemMedia>
                            <UiItemContent class="min-w-0">
                                <UiItemTitle class="max-w-full">
                                    <span class="truncate">{{ item.file.name }}</span>
                                </UiItemTitle>
                                <UiItemDescription :class="item.error && 'text-destructive'">
                                    {{ item.error || filesize(item.file.size, { locale }) }}
                                </UiItemDescription>
                            </UiItemContent>
                            <UiItemActions>
                                <UiButton
                                    variant="ghost"
                                    size="icon-sm"
                                    :aria-label="$t('shareTarget.remove')"
                                    :disabled="isSaving"
                                    @click="handleRemove(item)"
                                >
                                    <IX />
                                </UiButton>
                            </UiItemActions>
                        </UiItem>
                    </div>

                    <UploadOptions
                        v-if="canUpload"
                        v-model:folder="settings.folder"
                        v-model:file-name-type="settings.fileNameType"
                        v-model:expiration="expiration"
                        v-model:compression="settings.compression"
                        v-model:max-views="settings.maxViews"
                        v-model:password="settings.password"
                    />
                </UiCardContent>
            </UiCard>

            <UiCard v-for="(item, index) in texts" :key="item.key">
                <UiCardHeader>
                    <UiCardTitle>
                        {{ item.url ? $t('shareTarget.link') : $t('shareTarget.text') }}
                    </UiCardTitle>
                    <UiCardDescription class="line-clamp-1 break-all">
                        {{ item.url || item.content }}
                    </UiCardDescription>
                    <UiCardAction>
                        <UiButton
                            variant="ghost"
                            size="icon-sm"
                            :aria-label="$t('shareTarget.remove')"
                            :disabled="isSaving"
                            @click="handleRemove(item)"
                        >
                            <IX />
                        </UiButton>
                    </UiCardAction>
                </UiCardHeader>
                <UiCardContent class="flex flex-col gap-6">
                    <TextBlock v-if="!item.action" variant="muted">
                        {{ $t('shareTarget.cannotSave') }}
                    </TextBlock>

                    <template v-else>
                        <UiTabs v-if="item.url && canShorten && canNote" v-model="item.action">
                            <UiTabsList class="w-full">
                                <UiTabsTrigger value="link">
                                    <ILink />
                                    {{ $t('shareTarget.shorten') }}
                                </UiTabsTrigger>
                                <UiTabsTrigger value="note">
                                    <IBookmark />
                                    {{ $t('shareTarget.saveAsNote') }}
                                </UiTabsTrigger>
                            </UiTabsList>
                        </UiTabs>

                        <TextBlock v-if="item.action === 'link'" variant="muted">
                            {{ $t('shareTarget.shortenDescription') }}
                        </TextBlock>

                        <UiFieldGroup v-else>
                            <UiField>
                                <UiFieldLabel :for="`share-title-${index}`">
                                    {{ $t('shareTarget.noteTitle') }}
                                </UiFieldLabel>
                                <UiInput
                                    :id="`share-title-${index}`"
                                    v-model="item.title"
                                    :maxlength="48"
                                />
                            </UiField>
                            <UiField>
                                <UiFieldLabel :for="`share-content-${index}`">
                                    {{ $t('shareTarget.noteContent') }}
                                </UiFieldLabel>
                                <UiTextarea
                                    :id="`share-content-${index}`"
                                    v-model="item.content"
                                    class="min-h-32"
                                />
                            </UiField>
                            <UiField
                                orientation="horizontal"
                                class="has-[>[data-slot=field-content]]:items-center"
                            >
                                <UiFieldContent>
                                    <UiFieldLabel :for="`share-public-${index}`">
                                        {{ $t('shareTarget.publicNote') }}
                                    </UiFieldLabel>
                                    <UiFieldDescription>
                                        {{ $t('shareTarget.publicNoteDescription') }}
                                    </UiFieldDescription>
                                </UiFieldContent>
                                <UiSwitch :id="`share-public-${index}`" v-model="item.public" />
                            </UiField>
                        </UiFieldGroup>

                        <UiAlert v-if="item.error" variant="destructive">
                            <ICircleAlert />
                            <UiAlertTitle>{{ item.error }}</UiAlertTitle>
                        </UiAlert>
                    </template>
                </UiCardContent>
            </UiCard>

            <div v-if="hasPending" class="flex flex-col-reverse gap-2 sm:flex-row sm:justify-end">
                <UiButton variant="outline" :disabled="isSaving" @click="handleDiscard">
                    {{ $t('shareTarget.discard') }}
                </UiButton>
                <UiButton :disabled="isSaving || !canSave" @click="handleSave">
                    <UiSpinner v-if="isSaving" />
                    {{ $t('shareTarget.save') }}
                </UiButton>
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
import { filesize } from 'filesize';

import { IBookmark, IFile, ILink, IShare2 } from '#components';

import type { LinkData } from '~/types/links';
import type { NoteData } from '~/types/notes';

const SHARE_CACHE = 'fileship-share';
const URL_PATTERN = /https?:\/\/[^\s<>"']+/;

const route = useRoute();
const currentUser = useAuthUser();
const requestUrl = useRequestURL();
const { upload, dismiss } = useUploader();
const { $toast } = useNuxtApp();
const { t, locale } = useI18n();
const { copy } = useClipboard({ legacy: true });

const isLoading = ref(true);
const isSaving = ref(false);
const receiveFailed = ref(false);
const files = ref<SharedFile[]>([]);
const texts = ref<SharedText[]>([]);
const results = ref<ShareResult[]>([]);
const expiration = ref('labels.expiration.never');

const settings = reactive({
    fileNameType: 'Random' as NonNullable<FileSettings['fileNameType']>,
    maxViews: 0,
    password: '',
    compression: 0,
    folder: null as string | null,
});

const resultIcons = { file: IFile, link: ILink, note: IBookmark };

const canUpload = computed(() => canUploadFiles(currentUser));
const canShorten = computed(() => canShortenLinks(currentUser));
const canNote = computed(() => canTakeNotes(currentUser));

const hasPending = computed(() => !!files.value.length || !!texts.value.length);

const isValidNote = (item: SharedText) => item.title.trim().length >= 3 && !!item.content;

const canSave = computed(
    () =>
        ((canUpload.value && !!files.value.length) || texts.value.some((item) => item.action)) &&
        texts.value.every((item) => item.action !== 'note' || isValidNote(item)),
);

const openShareCache = () => caches.open(SHARE_CACHE);

const createText = (
    key: string,
    shared: { title: string; text: string; url: string },
): SharedText => {
    const url = shared.url || shared.text.match(URL_PATTERN)?.[0] || '';
    const content = [shared.text, !shared.text.includes(shared.url) && shared.url]
        .filter(Boolean)
        .join('\n\n');
    const title = shared.title || (url && URL.parse(url)?.hostname) || content.split('\n')[0]!;
    const actions = (['link', 'note'] as const).filter((action) =>
        action === 'link' ? !!url && canShorten.value : canNote.value,
    );

    return {
        key,
        url,
        title: title.trim().slice(0, 48),
        content,
        public: false,
        action: (content === url ? actions[0] : actions.at(-1)) ?? null,
        error: null,
    };
};

const loadShared = async () => {
    const cache = await openShareCache();

    for (const request of await cache.keys()) {
        const response = (await cache.match(request))!;

        if (request.url.endsWith('/text')) {
            texts.value.push(createText(request.url, await response.json()));
            continue;
        }

        files.value.push({
            key: request.url,
            file: new File(
                [await response.blob()],
                decodeURIComponent(response.headers.get('X-File-Name')!),
                {
                    type: response.headers.get('Content-Type')!,
                    lastModified: Number(response.headers.get('X-Last-Modified')),
                },
            ),
            error: null,
        });
    }
};

const forget = async (item: SharedFile | SharedText) => {
    files.value = files.value.filter((entry) => entry !== item);
    texts.value = texts.value.filter((entry) => entry !== item);

    await (await openShareCache()).delete(item.key);
};

const saveFiles = async () => {
    const pending = files.value;
    pending.forEach((item) => (item.error = null));

    const { results: urls } = await upload(
        pending.map((item) => item.file),
        {
            ...settings,
            password: settings.password || null,
            expiration: EXPIRATIONS.find((option) => option.label === expiration.value)!.value,
        },
    );

    await Promise.all(
        pending.map(async (item, index) => {
            const url = urls[index];

            if (typeof url === 'string') {
                results.value.push({ type: 'file', name: item.file.name, url });
                await forget(item);
                return;
            }

            item.error =
                (item.file as UploadingFileData).status?.error || t('upload.errors.failed');
            dismiss(item.file);
        }),
    );
};

const saveText = async (item: SharedText) => {
    item.error = null;

    try {
        if (item.action === 'link') {
            const link = await $fetch<LinkData>('/api/links', {
                method: 'POST',
                body: { url: item.url },
            });

            results.value.push({ type: 'link', name: item.url, url: link.shortUrl });
        } else {
            const note = await $fetch<NoteData>('/api/notes', {
                method: 'POST',
                body: { title: item.title, content: item.content, public: item.public },
            });

            results.value.push({
                type: 'note',
                name: note.title,
                url: note.publicUrl || `${requestUrl.origin}/dashboard/notes/${note.id}`,
            });
        }

        await forget(item);
    } catch (error: any) {
        const formErrors = error.data?.data?.formErrors;

        item.error =
            formErrors?.url?._errors[0] ||
            formErrors?.title?._errors[0] ||
            formErrors?.content?._errors[0] ||
            getErrorMessage(error);
    }
};

const handleSave = async () => {
    isSaving.value = true;

    await Promise.all([
        ...(canUpload.value && files.value.length ? [saveFiles()] : []),
        ...texts.value.filter((item) => item.action).map(saveText),
    ]);

    isSaving.value = false;

    if ([...files.value, ...texts.value].some((item) => item.error)) {
        $toast.error(t('shareTarget.someFailed'));
    } else {
        $toast.success(t('shareTarget.saved'));
    }
};

const handleRemove = (item: SharedFile | SharedText) => forget(item);

const handleDiscard = () =>
    askConfirmation({
        title: t('shareTarget.discardTitle'),
        description: t('shareTarget.discardDescription'),
        confirmText: t('shareTarget.discard'),
        destructive: true,
        action: () => Promise.all([...files.value, ...texts.value].map(forget)),
    });

const handleCopy = (url: string) => {
    copy(url);
    $toast.success(t('shareTarget.copied'));
};

const handleCopyAll = () => {
    copy(results.value.map((result) => result.url).join('\n'));
    $toast.success(t('shareTarget.allCopied'));
};

onMounted(async () => {
    receiveFailed.value = !!route.query.failed;
    if (receiveFailed.value) navigateTo({ query: {} }, { replace: true });

    if ('caches' in window) await loadShared();
    isLoading.value = false;
});

interface SharedFile {
    key: string;
    file: File;
    error: string | null;
}

interface SharedText {
    key: string;
    url: string;
    title: string;
    content: string;
    public: boolean;
    action: 'link' | 'note' | null;
    error: string | null;
}

interface ShareResult {
    type: 'file' | 'link' | 'note';
    name: string;
    url: string;
}

definePageMeta({
    layout: 'dashboard',
    middleware: 'user-only',
});
</script>
