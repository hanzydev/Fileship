<template>
    <main>
        <div v-if="currentUser">
            <Centered v-if="currentUser.backupRestoreState">
                <UiCard class="w-full max-w-sm">
                    <UiCardHeader class="items-center text-center">
                        <UiSpinner class="mx-auto size-8" />
                        <UiCardTitle>{{ $t('layout.restore.title') }}</UiCardTitle>
                        <UiCardDescription>
                            {{ $t(restoreStates[currentUser.backupRestoreState]) }}
                        </UiCardDescription>
                    </UiCardHeader>
                </UiCard>
            </Centered>

            <UiSidebarProvider v-else storage-key="sidebar">
                <AppSidebar />
                <UiSidebarInset>
                    <div
                        v-if="currentUser.impersonator"
                        class="flex flex-wrap items-center justify-center gap-x-4 gap-y-2 bg-primary px-4 py-2 text-sm text-primary-foreground"
                    >
                        <span class="flex items-center gap-2">
                            <IVenetianMask class="size-4" />
                            <i18n-t keypath="layout.impersonation.signedInAs" tag="span">
                                <template #username>
                                    <strong>{{ currentUser.username }}</strong>
                                </template>
                            </i18n-t>
                        </span>
                        <UiButton
                            size="xs"
                            variant="secondary"
                            :disabled="isReturning"
                            @click="handleStopImpersonating"
                        >
                            <UiSpinner v-if="isReturning" />
                            {{ $t('layout.impersonation.return') }}
                        </UiButton>
                    </div>
                    <AppHeader :labels="breadcrumbLabels">
                        <UiButton v-if="canUpload" @click="uploadSheetOpen = true">
                            <IUpload />
                            <span class="hidden sm:inline">{{ $t('layout.upload') }}</span>
                        </UiButton>
                        <UiButton
                            variant="outline"
                            class="hidden w-56 justify-start sm:flex"
                            @click="commandPaletteOpen = true"
                        >
                            <ISearch class="text-muted-foreground" />
                            <span class="font-normal text-muted-foreground">
                                {{ $t('layout.search') }}
                            </span>
                            <UiKbdGroup class="ml-auto">
                                <UiKbd v-for="key in paletteShortcut" :key>{{ key }}</UiKbd>
                            </UiKbdGroup>
                        </UiButton>
                        <UiButton
                            variant="ghost"
                            size="icon"
                            class="sm:hidden"
                            :aria-label="$t('layout.palette.title')"
                            @click="commandPaletteOpen = true"
                        >
                            <ISearch />
                        </UiButton>
                    </AppHeader>

                    <slot />
                </UiSidebarInset>
            </UiSidebarProvider>

            <UiCommandDialog
                v-model:open="commandPaletteOpen"
                :title="$t('layout.palette.title')"
                :description="$t('layout.palette.description')"
            >
                <UiCommandInput
                    :placeholder="$t('layout.palette.placeholder')"
                    @update:model-value="paletteQuery = $event"
                />
                <UiCommandList class="mb-1">
                    <UiCommandEmpty>{{ $t('layout.palette.empty') }}</UiCommandEmpty>

                    <UiCommandGroup
                        v-if="paletteTerm && matchedFiles.length"
                        class="px-1 pb-0"
                        :heading="$t('layout.pages.files')"
                    >
                        <UiCommandItem
                            v-for="file in matchedFiles"
                            :key="`${file.id}:${paletteTerm}`"
                            :value="`file:${file.id}`"
                            @select="handlePaletteSelect(`/dashboard/files?file=${file.id}`)"
                        >
                            <IFile />
                            <span class="truncate">{{ file.name }}</span>
                            <UiCommandShortcut>{{ file.size.formatted }}</UiCommandShortcut>
                        </UiCommandItem>
                    </UiCommandGroup>

                    <UiCommandGroup
                        v-if="paletteTerm && matchedFolders.length"
                        class="px-1 pb-0"
                        :heading="$t('layout.pages.folders')"
                    >
                        <UiCommandItem
                            v-for="folder in matchedFolders"
                            :key="`${folder.id}:${paletteTerm}`"
                            :value="`folder:${folder.id}`"
                            @select="handlePaletteSelect(`/dashboard/folders/${folder.id}`)"
                        >
                            <IFolder />
                            <span class="truncate">{{ folder.name }}</span>
                        </UiCommandItem>
                    </UiCommandGroup>

                    <UiCommandGroup
                        v-if="paletteTerm && matchedNotes.length"
                        class="px-1 pb-0"
                        :heading="$t('layout.pages.notes')"
                    >
                        <UiCommandItem
                            v-for="note in matchedNotes"
                            :key="`${note.id}:${paletteTerm}`"
                            :value="`note:${note.id}`"
                            @select="handlePaletteSelect(`/dashboard/notes/${note.id}`)"
                        >
                            <IBookmark />
                            <span class="truncate">{{ note.title }}</span>
                        </UiCommandItem>
                    </UiCommandGroup>

                    <UiCommandGroup
                        v-if="paletteTerm && matchedLinks.length"
                        class="px-1 pb-0"
                        :heading="$t('layout.pages.links')"
                    >
                        <UiCommandItem
                            v-for="link in matchedLinks"
                            :key="`${link.id}:${paletteTerm}`"
                            :value="`link:${link.id}`"
                            @select="
                                handlePaletteSelect(
                                    `/dashboard/links?q=${encodeURIComponent(link.code)}`,
                                )
                            "
                        >
                            <ILink />
                            <span class="truncate">{{ link.code }}</span>
                            <UiCommandShortcut class="max-w-48 truncate">
                                {{ link.url }}
                            </UiCommandShortcut>
                        </UiCommandItem>
                    </UiCommandGroup>

                    <UiCommandGroup
                        v-if="paletteTerm && matchedUsers.length"
                        class="px-1 pb-0"
                        :heading="$t('layout.pages.users')"
                    >
                        <UiCommandItem
                            v-for="user in matchedUsers"
                            :key="`${user.id}:${paletteTerm}`"
                            :value="`user:${user.id}`"
                            @select="handlePaletteSelect(`/dashboard/users?user=${user.id}`)"
                        >
                            <IUser />
                            <span class="truncate">{{ user.username }}</span>
                        </UiCommandItem>
                    </UiCommandGroup>

                    <UiCommandGroup class="px-1 pb-0" :heading="$t('layout.palette.actions')">
                        <UiCommandItem
                            v-for="action in paletteActions"
                            :key="action.label"
                            :value="action.label"
                            @select="handlePaletteAction(action.run)"
                        >
                            <component :is="action.icon" />
                            {{ action.label }}
                            <UiCommandShortcut v-if="action.shortcut">
                                {{ action.shortcut }}
                            </UiCommandShortcut>
                        </UiCommandItem>
                    </UiCommandGroup>

                    <UiCommandSeparator />

                    <UiCommandGroup class="px-1 pb-0" :heading="$t('layout.palette.goTo')">
                        <UiCommandItem
                            v-for="page in palettePages"
                            :key="page.url"
                            :value="page.label"
                            @select="handlePaletteSelect(page.url)"
                        >
                            <component :is="page.icon" />
                            {{ page.label }}
                        </UiCommandItem>
                    </UiCommandGroup>
                </UiCommandList>
            </UiCommandDialog>
            <ConfirmDialog />
            <UiDialog :open="!!verification" @update:open="!$event && verification?.cancel()">
                <UiDialogContent
                    v-if="currentVerification"
                    :key="verifyOpenCount"
                    class="sm:max-w-sm"
                >
                    <form class="flex flex-col gap-6" @submit.prevent="handleVerify">
                        <UiDialogHeader>
                            <UiDialogTitle>{{ $t('layout.verify.title') }}</UiDialogTitle>
                            <UiDialogDescription>
                                {{ $t('layout.verify.description') }}
                            </UiDialogDescription>
                        </UiDialogHeader>

                        <UiTabs v-if="verifyMethods.length > 1" v-model="verifyMethod">
                            <UiTabsList class="w-full">
                                <UiTabsTrigger
                                    v-for="m in verifyMethods"
                                    :key="m.type"
                                    :value="m.type"
                                >
                                    {{ $t(verifyLabels[m.type]) }}
                                </UiTabsTrigger>
                            </UiTabsList>
                        </UiTabs>

                        <TextBlock v-if="verifyMethod === 'passkey'" variant="muted">
                            {{ $t('layout.verify.passkeyHint') }}
                        </TextBlock>

                        <UiField v-else-if="verifyMethod === 'totp'">
                            <UiFieldLabel>{{ $t('layout.verify.authenticatorCode') }}</UiFieldLabel>
                            <OtpInput v-model="verifyTotp" @complete="handleVerify" />
                        </UiField>

                        <UiField v-else>
                            <UiFieldLabel for="verify-password">
                                {{ $t('layout.verify.password') }}
                            </UiFieldLabel>
                            <PasswordInput
                                id="verify-password"
                                v-model="verifyPassword"
                                autocomplete="current-password"
                            />
                        </UiField>

                        <UiAlert v-if="currentVerification.error" variant="destructive">
                            <ICircleAlert />
                            <UiAlertTitle>{{ currentVerification.error }}</UiAlertTitle>
                        </UiAlert>

                        <UiDialogFooter>
                            <UiButton
                                type="button"
                                variant="outline"
                                @click="currentVerification.cancel()"
                            >
                                {{ $t('common.cancel') }}
                            </UiButton>
                            <UiButton
                                v-if="verifyMethod === 'passkey'"
                                type="button"
                                :disabled="currentVerification.isSubmitting"
                                @click="handleVerifyPasskey"
                            >
                                <UiSpinner v-if="currentVerification.isSubmitting" />
                                <IFingerprint v-else />
                                {{ $t('layout.verify.usePasskey') }}
                            </UiButton>
                            <UiButton
                                v-else
                                type="submit"
                                :disabled="currentVerification.isSubmitting || !canVerify"
                            >
                                <UiSpinner v-if="currentVerification.isSubmitting" />
                                {{ $t('layout.verify.submit') }}
                            </UiButton>
                        </UiDialogFooter>
                    </form>
                </UiDialogContent>
            </UiDialog>
            <UploadSheet v-if="canUpload" ref="uploadSheet" />
            <Transition
                enter-active-class="animate-in fade-in slide-in-from-bottom-4 duration-200"
                leave-active-class="animate-out fade-out slide-out-to-bottom-4 duration-200"
            >
                <UiCard
                    v-if="uploadingFiles.length"
                    size="sm"
                    class="fixed right-4 bottom-4 z-40 w-[calc(100%-2rem)] sm:w-96"
                >
                    <UiCollapsible v-model:open="isQueueExpanded" class="flex flex-col gap-4">
                        <UiCardHeader>
                            <UiCardTitle class="flex items-center gap-2">
                                <UiSpinner v-if="activeUploads" />
                                <ICircleAlert v-else class="size-4 text-destructive" />
                                {{ queueTitle }}
                            </UiCardTitle>
                            <UiCardAction>
                                <UiCollapsibleTrigger as-child>
                                    <UiButton
                                        variant="ghost"
                                        size="icon-sm"
                                        :aria-label="
                                            isQueueExpanded
                                                ? $t('layout.queue.collapse')
                                                : $t('layout.queue.expand')
                                        "
                                    >
                                        <IChevronDown
                                            :class="
                                                cn(
                                                    'transition-transform',
                                                    !isQueueExpanded && 'rotate-180',
                                                )
                                            "
                                        />
                                    </UiButton>
                                </UiCollapsibleTrigger>
                            </UiCardAction>
                        </UiCardHeader>
                        <UiCollapsibleContent>
                            <UiCardContent class="flex max-h-72 flex-col gap-2 overflow-y-auto">
                                <UiItem
                                    v-for="(file, index) in uploadingFiles"
                                    :key="`${file.name}-${index}`"
                                    variant="muted"
                                    size="sm"
                                >
                                    <UiItemContent class="min-w-0">
                                        <UiItemTitle class="max-w-full">
                                            <span class="truncate">{{ file.name }}</span>
                                        </UiItemTitle>
                                        <UiItemDescription
                                            :class="
                                                cn(
                                                    'truncate',
                                                    file.status!.error && 'text-destructive',
                                                )
                                            "
                                        >
                                            <template v-if="file.status!.error">
                                                {{ file.status!.error }}
                                            </template>
                                            <template v-else-if="!file.status!.started">
                                                {{ $t('layout.queue.waiting') }}
                                            </template>
                                            <template v-else-if="!file.status!.completed">
                                                {{
                                                    $t('layout.queue.progress', {
                                                        speed: filesize(
                                                            file.status!.progress.speed,
                                                            { locale },
                                                        ),
                                                        eta: formatEta(file.status!.progress.eta),
                                                    })
                                                }}
                                            </template>
                                            <template v-else>
                                                {{ $t('layout.queue.uploaded') }}
                                            </template>
                                        </UiItemDescription>
                                    </UiItemContent>
                                    <UiItemActions>
                                        <template v-if="file.status!.error">
                                            <UiButton
                                                variant="ghost"
                                                size="icon-sm"
                                                :aria-label="$t('layout.queue.retry')"
                                                @click="retry(file)"
                                            >
                                                <IRotateCw />
                                            </UiButton>
                                            <UiButton
                                                variant="ghost"
                                                size="icon-sm"
                                                :aria-label="$t('layout.queue.dismiss')"
                                                @click="dismiss(file)"
                                            >
                                                <IX />
                                            </UiButton>
                                        </template>
                                        <ICheck
                                            v-else-if="file.status!.completed"
                                            class="size-4 text-primary"
                                        />
                                        <UiItemDescription v-else>
                                            {{ file.status!.progress.percent }}%
                                        </UiItemDescription>
                                    </UiItemActions>
                                    <UiItemFooter
                                        v-if="!file.status!.error && !file.status!.completed"
                                    >
                                        <UiProgress
                                            class="flex-1"
                                            :model-value="file.status!.progress.percent"
                                        />
                                    </UiItemFooter>
                                </UiItem>
                            </UiCardContent>
                        </UiCollapsibleContent>
                    </UiCollapsible>
                </UiCard>
            </Transition>

            <Transition
                enter-active-class="animate-in fade-in-0 duration-100"
                leave-active-class="animate-out fade-out-0 duration-100"
            >
                <div
                    v-if="isOverDropZone && canUpload && !uploadSheetOpen"
                    class="pointer-events-none fixed inset-0 z-50 flex items-center justify-center bg-background/80 backdrop-blur-sm"
                >
                    <EmptyState
                        :icon="IUpload"
                        :title="$t('layout.drop.title')"
                        :description="
                            currentFolderName
                                ? $t('layout.drop.toFolder', { folder: currentFolderName })
                                : $t('layout.drop.copyLinks')
                        "
                    />
                </div>
            </Transition>
        </div>
    </main>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';
import { filesize } from 'filesize';

import { startAuthentication } from '@simplewebauthn/browser';

import {
    IBookmark,
    IChartNoAxesColumn,
    IClipboardList,
    IFiles,
    IFolder,
    IFolderPlus,
    IHome,
    ILink,
    IMoon,
    IServer,
    ISettings,
    ISquarePen,
    ISun,
    ITrash2,
    IUpload,
    IUsers,
} from '#components';

import type { FileData } from '~/types/files';
import type { FolderData } from '~/types/folders';
import type { LinkData } from '~/types/links';
import type { NoteData } from '~/types/notes';
import type { UserData } from '~/types/users';

const embed = useEmbed();
const domains = useDomains();
const currentUser = useAuthUser();
const route = useRoute();
const { $toast } = useNuxtApp();
const { t, locale } = useI18n();
const router = useRouter();
const uploadSheetOpen = useUploadSheet();
const uploadingFiles = useUploadingFiles();
const { quickUpload, retry, dismiss } = useUploader();
const verification = useVerification();

const uploadSheet = useTemplateRef('uploadSheet');
const commandPaletteOpen = ref(false);
const isMac = ref(false);

const paletteShortcut = computed(() => [isMac.value ? '⌘' : 'Ctrl', 'K']);
const isReturning = ref(false);
const { stopImpersonating } = useImpersonation();

const restoreStates = {
    Extracting: 'layout.restore.extracting',
    DeletingPreviousData: 'layout.restore.deletingPreviousData',
    RestoringData: 'layout.restore.restoringData',
};

const breadcrumbLabels = {
    dashboard: 'layout.pages.home',
    files: 'layout.pages.files',
    folders: 'layout.pages.folders',
    notes: 'layout.pages.notes',
    new: 'layout.pages.newNote',
    links: 'layout.pages.links',
    trash: 'layout.pages.trash',
    stats: 'layout.pages.stats',
    users: 'layout.pages.users',
    logs: 'layout.pages.logs',
    instance: 'layout.pages.instance',
    settings: 'layout.pages.settings',
    profile: 'settings.nav.profile',
    security: 'settings.nav.security',
    sessions: 'settings.nav.sessions',
    backups: 'settings.nav.backups',
    sharing: 'settings.nav.sharing',
    inbox: 'settings.nav.inbox',
    integrations: 'settings.nav.integrations',
    ai: 'settings.nav.ai',
    appearance: 'settings.nav.appearance',
};

const breadcrumbItems = useBreadcrumbs(breadcrumbLabels);

const canUpload = computed(() => canUploadFiles(currentUser.value));

const currentFolderName = computed(() =>
    route.path.startsWith('/dashboard/folders/') ? breadcrumbItems.value.at(-1)!.label : null,
);

const handleStopImpersonating = async () => {
    isReturning.value = true;
    await stopImpersonating().catch((error) => $toast.error(getErrorMessage(error)));
    isReturning.value = false;
};

const isTyping = () =>
    !!document.activeElement?.closest('input, textarea, select, [contenteditable="true"]');

const handleFiles = (files: File[] | null, source: 'drag-drop' | 'paste') => {
    if (!files?.length || !canUpload.value) return;

    if (uploadSheetOpen.value) uploadSheet.value!.addFiles(files);
    else quickUpload(files, source);
};

const { isOverDropZone } = useDropZone(import.meta.client ? document.body : null, {
    onDrop: (files) => handleFiles(files, 'drag-drop'),
    checkValidity: (items) => [...items].some((item) => item.kind === 'file'),
    multiple: true,
    preventDefaultForUnhandled: false,
});

useEventListener(window, 'paste', (event: ClipboardEvent) => {
    const items = event.clipboardData?.items;
    if (!items) return;

    const files: File[] = [];

    for (const item of items) {
        if (item.kind === 'file' && item.type.startsWith('image/')) {
            const file = item.getAsFile();
            if (file) files.push(file);
        }
    }

    handleFiles(files, 'paste');
});

onKeyStroke(['u', 'U'], (event) => {
    if (event.ctrlKey || event.metaKey || event.altKey || isTyping() || !canUpload.value) return;

    event.preventDefault();
    uploadSheetOpen.value = true;
});

watch(
    () => route.query.upload,
    (value) => {
        if (value !== '1') return;

        uploadSheetOpen.value = true;
        router.replace({ query: { ...route.query, upload: undefined } });
    },
    { immediate: true },
);

const trash = useTrash();
const colorMode = useColorMode();

const paletteQuery = ref('');

const paletteTerm = computed(() => paletteQuery.value.trim());

const useMatches = <T extends { id: string }>(
    resource: ListResource,
    endpoint: string,
    isAllowed: () => boolean,
) => {
    const { items, isFresh } = usePaginatedList<T>(
        resource,
        endpoint,
        () => ({ q: paletteTerm.value }),
        {
            limit: 5,
            enabled: () => commandPaletteOpen.value && !!paletteTerm.value && isAllowed(),
        },
    );

    return computed(() => (isFresh.value ? items.value : []));
};

const matchedFiles = useMatches<FileData>('files', '/api/files', () =>
    canUploadFiles(currentUser.value),
);

const matchedFolders = useMatches<FolderData>('folders', '/api/folders', () =>
    canUploadFiles(currentUser.value),
);

const matchedNotes = useMatches<NoteData>('notes', '/api/notes', () =>
    canTakeNotes(currentUser.value),
);

const matchedLinks = useMatches<LinkData>('links', '/api/links', () =>
    canShortenLinks(currentUser.value),
);

const matchedUsers = useMatches<UserData>('users', '/api/users', () => isAdmin(currentUser.value));

const palettePages = computed(() => [
    { label: t('layout.pages.home'), url: '/dashboard', icon: IHome },
    ...(canUploadFiles(currentUser.value)
        ? [
              { label: t('layout.pages.files'), url: '/dashboard/files', icon: IFiles },
              { label: t('layout.pages.folders'), url: '/dashboard/folders', icon: IFolder },
          ]
        : []),
    ...(canTakeNotes(currentUser.value)
        ? [{ label: t('layout.pages.notes'), url: '/dashboard/notes', icon: IBookmark }]
        : []),
    ...(canShortenLinks(currentUser.value)
        ? [{ label: t('layout.pages.links'), url: '/dashboard/links', icon: ILink }]
        : []),
    ...(trash.isEnabled.value &&
    (canUploadFiles(currentUser.value) || canTakeNotes(currentUser.value))
        ? [{ label: t('layout.pages.trash'), url: '/dashboard/trash', icon: ITrash2 }]
        : []),
    ...(isAdmin(currentUser.value)
        ? [
              { label: t('layout.pages.stats'), url: '/dashboard/stats', icon: IChartNoAxesColumn },
              { label: t('layout.pages.users'), url: '/dashboard/users', icon: IUsers },
              { label: t('layout.pages.logs'), url: '/dashboard/logs', icon: IClipboardList },
              ...(currentUser.value?.superAdmin
                  ? [
                        {
                            label: t('layout.pages.instance'),
                            url: '/dashboard/instance',
                            icon: IServer,
                        },
                    ]
                  : []),
          ]
        : []),
    { label: t('layout.pages.settings'), url: '/dashboard/settings/profile', icon: ISettings },
]);

const paletteActions = computed(() => [
    ...(canUploadFiles(currentUser.value)
        ? [
              {
                  label: t('layout.palette.uploadFiles'),
                  icon: IUpload,
                  shortcut: 'U',
                  run: () => (uploadSheetOpen.value = true),
              },
              {
                  label: t('layout.palette.newFolder'),
                  icon: IFolderPlus,
                  run: () => navigateTo('/dashboard/folders?new=1'),
              },
          ]
        : []),
    ...(canTakeNotes(currentUser.value)
        ? [
              {
                  label: t('layout.palette.newNote'),
                  icon: ISquarePen,
                  run: () => navigateTo('/dashboard/notes/new'),
              },
          ]
        : []),
    ...(canShortenLinks(currentUser.value)
        ? [
              {
                  label: t('layout.palette.shortenLink'),
                  icon: ILink,
                  run: () => navigateTo('/dashboard/links?new=1'),
              },
          ]
        : []),
    {
        label:
            colorMode.value === 'dark'
                ? t('layout.palette.lightMode')
                : t('layout.palette.darkMode'),
        icon: colorMode.value === 'dark' ? ISun : IMoon,
        run: () => (colorMode.preference = colorMode.value === 'dark' ? 'light' : 'dark'),
    },
]);

const handlePaletteSelect = (url: string) => {
    commandPaletteOpen.value = false;
    navigateTo(url);
};

const handlePaletteAction = (run: () => unknown) => {
    commandPaletteOpen.value = false;
    run();
};

watch(commandPaletteOpen, (value) => value && (paletteQuery.value = ''));

onKeyStroke(['k', 'K'], (event) => {
    if (event.ctrlKey || event.metaKey) {
        event.preventDefault();
        commandPaletteOpen.value = !commandPaletteOpen.value;
    }
});

const currentVerification = ref(verification.value);
const verifyOpenCount = ref(0);

const verifyLabels = {
    passkey: 'layout.verify.passkey',
    totp: 'layout.verify.code',
    password: 'layout.verify.password',
};

const verifyMethod = ref<VerificationMethod['type']>('password');
const verifyTotp = ref('');
const verifyPassword = ref('');

const verifyMethods = computed(() => currentVerification.value?.methods || []);

const canVerify = computed(() =>
    verifyMethod.value === 'totp' ? verifyTotp.value.length === 6 : !!verifyPassword.value,
);

const handleVerify = () => {
    if (!canVerify.value || verification.value!.isSubmitting) return;

    verification.value!.submit({
        type: verifyMethod.value,
        data: verifyMethod.value === 'totp' ? verifyTotp.value : verifyPassword.value,
    });
};

const handleVerifyPasskey = async () => {
    const { ticket, authenticationOptions } = verifyMethods.value.find(
        (m) => m.type === 'passkey',
    ) as any;

    currentVerification.value!.isSubmitting = true;

    try {
        const authenticationResponse = await startAuthentication({
            optionsJSON: authenticationOptions,
        });

        verification.value!.submit({ type: 'passkey', data: { ticket, authenticationResponse } });
    } catch {
        currentVerification.value!.isSubmitting = false;
        $toast.error(t('layout.verify.passkeyFailed'));
    }
};

watch(verification, (value, oldValue) => {
    if (!value) return;

    currentVerification.value = value;
    if (!oldValue) verifyOpenCount.value++;
});

watch(
    () => verification.value?.error,
    (value) => value && (verifyTotp.value = ''),
);

watch(verifyMethods, (value) => {
    if (!value.length) return;

    verifyTotp.value = '';
    verifyPassword.value = '';
    verifyMethod.value = value.some((m) => m.type === 'passkey')
        ? 'passkey'
        : value.some((m) => m.type === 'totp')
          ? 'totp'
          : 'password';
});

const isQueueExpanded = ref(true);

const activeUploads = computed(() => uploadingFiles.value.filter((f) => !f.status!.error).length);
const failedUploads = computed(() => uploadingFiles.value.length - activeUploads.value);

const queueTitle = computed(() =>
    activeUploads.value
        ? t('layout.queue.uploading', activeUploads.value)
        : t('layout.queue.failed', failedUploads.value),
);

const formatEta = (seconds: number) =>
    dayjs.duration(seconds, 'seconds').locale(locale.value).humanize();

onMounted(async () => {
    isMac.value = /Mac|iPhone|iPad/.test(navigator.userAgent);

    await Promise.all([
        $fetch<IEmbed>('/api/users/@me/embed').then((data) => (embed.value = data)),
        $fetch<string[]>('/api/users/@me/domains').then((data) => (domains.value = data)),
    ]).catch((error) => $toast.error(getErrorMessage(error)));
});
</script>
