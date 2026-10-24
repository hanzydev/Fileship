<template>
    <UiSidebar collapsible="icon">
        <UiSidebarHeader>
            <UiSidebarMenu>
                <UiSidebarMenuItem>
                    <UiSidebarMenuButton
                        as-child
                        class="group-data-[collapsible=icon]:justify-center"
                        size="lg"
                        :tooltip="
                            updaterStatus
                                ? $t('sidebar.versionTooltip', { version: updaterStatus.version })
                                : 'Fileship'
                        "
                    >
                        <a
                            target="_blank"
                            rel="noopener noreferrer"
                            href="https://github.com/hanzydev/Fileship"
                            class="flex items-center gap-4"
                        >
                            <img
                                src="/fileship.png"
                                class="size-10 group-data-[collapsible=icon]:size-4"
                            />
                            <div class="flex flex-col group-data-[collapsible=icon]:hidden">
                                <TextBlock variant="large">Fileship</TextBlock>
                                <Transition
                                    enter-active-class="animate-in fade-in slide-in-from-top-2 zoom-in-95 duration-200"
                                    leave-active-class="animate-out fade-out slide-out-to-top-2 zoom-out-95 duration-200"
                                >
                                    <TextBlock v-if="updaterStatus" variant="muted">
                                        {{
                                            $t('sidebar.version', {
                                                version: updaterStatus.version,
                                            })
                                        }}
                                    </TextBlock>
                                </Transition>
                            </div>
                        </a>
                    </UiSidebarMenuButton>
                </UiSidebarMenuItem>
            </UiSidebarMenu>
        </UiSidebarHeader>
        <UiSidebarContent>
            <UiSidebarGroup v-for="category in items" :key="category.title">
                <UiSidebarGroupLabel>{{ category.title }}</UiSidebarGroupLabel>
                <UiSidebarGroupContent>
                    <UiSidebarMenu>
                        <UiSidebarMenuItem v-for="child in category.children" :key="child.title">
                            <UiSidebarMenuButton
                                as-child
                                :is-active="isActive(child.url)"
                                :tooltip="child.title"
                            >
                                <NuxtLink :to="child.url">
                                    <component :is="child.icon" />
                                    <span>{{ child.title }}</span>
                                </NuxtLink>
                            </UiSidebarMenuButton>
                        </UiSidebarMenuItem>
                    </UiSidebarMenu>
                </UiSidebarGroupContent>
            </UiSidebarGroup>
        </UiSidebarContent>
        <UiSidebarFooter>
            <NuxtLink
                v-if="storageUsage && canUploadFiles(authUser)"
                to="/dashboard/settings/storage"
                class="flex flex-col gap-2 rounded-md p-2 transition-colors hover:bg-sidebar-accent group-data-[collapsible=icon]:hidden"
            >
                <span class="flex items-center justify-between text-xs">
                    <span class="font-medium">{{ $t('storage.title') }}</span>
                    <span v-if="storageRatio !== null" class="text-muted-foreground tabular-nums">
                        {{ percentFormat.format(storageRatio) }}
                    </span>
                </span>
                <StorageMeter :used="storageUsage.used" :limit="storageUsage.limit" />
            </NuxtLink>
            <template v-if="updaterStatus?.hasUpdate">
                <UiCard size="sm" class="group-data-[collapsible=icon]:hidden">
                    <UiCardHeader>
                        <UiCardTitle class="flex items-center gap-2">
                            <ISparkles class="size-4 text-primary" />
                            {{ $t('sidebar.updateAvailable') }}
                        </UiCardTitle>
                        <UiCardDescription>
                            {{
                                $t('sidebar.versionChange', {
                                    current: updaterStatus.version,
                                    latest: updaterStatus.latestVersion,
                                })
                            }}
                        </UiCardDescription>
                    </UiCardHeader>
                    <UiCardFooter class="gap-2">
                        <UiButton v-if="canUpdate" size="sm" class="flex-1" @click="handleUpdate">
                            {{ $t('sidebar.update') }}
                        </UiButton>
                        <UiButton
                            size="sm"
                            :variant="canUpdate ? 'outline' : 'default'"
                            :class="!canUpdate && 'flex-1'"
                            as-child
                        >
                            <a :href="updaterStatus.url" target="_blank" rel="noopener noreferrer">
                                {{ $t('sidebar.changelog') }}
                            </a>
                        </UiButton>
                    </UiCardFooter>
                </UiCard>

                <UiDialog
                    :open="isUpdating"
                    @update:open="updateState === 'Error' && (isUpdating = $event)"
                >
                    <UiDialogContent
                        :show-close-button="updateState === 'Error'"
                        class="sm:max-w-sm"
                        @interact-outside.prevent
                    >
                        <UiDialogHeader class="items-center text-center">
                            <UiSpinner
                                v-if="updateState !== 'Success' && updateState !== 'Error'"
                                class="size-8"
                            />
                            <ICircleCheck
                                v-else-if="updateState === 'Success'"
                                class="size-8 text-primary"
                            />
                            <ICircleX v-else class="size-8 text-destructive" />
                            <UiDialogTitle>{{ $t('sidebar.updating') }}</UiDialogTitle>
                            <UiDialogDescription>
                                {{
                                    updateStates.includes(updateState)
                                        ? $t(`sidebar.updateStates.${updateState}`)
                                        : updateState
                                }}
                            </UiDialogDescription>
                        </UiDialogHeader>
                        <UiDialogFooter v-if="updateState === 'Error'">
                            <UiButton variant="outline" class="w-full" @click="isUpdating = false">
                                {{ $t('common.close') }}
                            </UiButton>
                        </UiDialogFooter>
                    </UiDialogContent>
                </UiDialog>
            </template>
            <UiDropdownMenu>
                <UiDropdownMenuTrigger as-child>
                    <UiSidebarMenuButton
                        size="lg"
                        :is-active="isActive('/dashboard/settings')"
                        :tooltip="$t('sidebar.account')"
                    >
                        <UiAvatar class="size-8">
                            <UiAvatarImage
                                v-if="authUser?.avatar"
                                :src="authUser.avatar"
                                :alt="authUser.username"
                            />
                            <UiAvatarFallback>{{ userInitials }}</UiAvatarFallback>
                        </UiAvatar>
                        <div class="grid flex-1 text-left text-sm leading-tight">
                            <span class="truncate font-medium">
                                {{ upperFirst(authUser!.username) }}
                            </span>
                            <span class="truncate text-xs text-muted-foreground">
                                {{ roleLabel }}
                            </span>
                        </div>
                        <IChevronsUpDown class="ml-auto" />
                    </UiSidebarMenuButton>
                </UiDropdownMenuTrigger>
                <UiDropdownMenuContent
                    side="top"
                    align="start"
                    class="w-(--reka-dropdown-menu-trigger-width) min-w-56"
                >
                    <UiDropdownMenuLabel class="flex items-center gap-2 font-normal">
                        <UiAvatar class="size-8">
                            <UiAvatarImage
                                v-if="authUser?.avatar"
                                :src="authUser.avatar"
                                :alt="authUser.username"
                            />
                            <UiAvatarFallback>{{ userInitials }}</UiAvatarFallback>
                        </UiAvatar>
                        <div class="grid flex-1 text-left text-sm leading-tight">
                            <span class="truncate font-medium text-foreground">
                                {{ upperFirst(authUser!.username) }}
                            </span>
                            <span class="truncate text-xs">{{ roleLabel }}</span>
                        </div>
                    </UiDropdownMenuLabel>
                    <UiDropdownMenuSeparator />
                    <UiDropdownMenuItem as-child>
                        <NuxtLink to="/dashboard/settings/profile">
                            <ISettings />
                            {{ $t('sidebar.settings') }}
                        </NuxtLink>
                    </UiDropdownMenuItem>
                    <UiDropdownMenuSub>
                        <UiDropdownMenuSubTrigger>
                            <IPalette />
                            {{ $t('sidebar.appearance') }}
                        </UiDropdownMenuSubTrigger>
                        <UiDropdownMenuSubContent>
                            <UiDropdownMenuLabel>{{ $t('sidebar.mode') }}</UiDropdownMenuLabel>
                            <UiDropdownMenuRadioGroup v-model="colorMode.preference">
                                <UiDropdownMenuRadioItem
                                    v-for="mode in colorModes"
                                    :key="mode.value"
                                    :value="mode.value"
                                    @select.prevent
                                >
                                    <component :is="mode.icon" />
                                    {{ $t(mode.label) }}
                                </UiDropdownMenuRadioItem>
                            </UiDropdownMenuRadioGroup>
                            <UiDropdownMenuSeparator />
                            <UiDropdownMenuLabel>{{ $t('sidebar.accent') }}</UiDropdownMenuLabel>
                            <UiDropdownMenuRadioGroup
                                :model-value="currentTheme in accents ? currentTheme : 'Fileship'"
                                @update:model-value="updateTheme($event as Accent)"
                            >
                                <UiDropdownMenuRadioItem
                                    v-for="(color, accent) in accents"
                                    :key="accent"
                                    :value="accent"
                                    @select.prevent
                                >
                                    <span
                                        class="size-3 rounded-full"
                                        :style="{ background: color }"
                                    />
                                    {{ $t(`sidebar.accents.${accent}`) }}
                                </UiDropdownMenuRadioItem>
                            </UiDropdownMenuRadioGroup>
                        </UiDropdownMenuSubContent>
                    </UiDropdownMenuSub>
                    <UiDropdownMenuSub>
                        <UiDropdownMenuSubTrigger>
                            <ILanguages />
                            {{ $t('sidebar.language') }}
                        </UiDropdownMenuSubTrigger>
                        <UiDropdownMenuSubContent>
                            <UiDropdownMenuRadioGroup
                                :model-value="authUser!.language ?? 'system'"
                                @update:model-value="handleLanguageChange"
                            >
                                <UiDropdownMenuRadioItem
                                    v-for="language in ['system', ...LANGUAGES]"
                                    :key="language"
                                    :value="language"
                                    @select.prevent
                                >
                                    {{ $t(`labels.language.${language}`) }}
                                </UiDropdownMenuRadioItem>
                            </UiDropdownMenuRadioGroup>
                        </UiDropdownMenuSubContent>
                    </UiDropdownMenuSub>
                    <UiDropdownMenuSeparator />
                    <UiDropdownMenuItem
                        variant="destructive"
                        :disabled="isLoggingOut"
                        @select.prevent="handleLogout"
                    >
                        <UiSpinner v-if="isLoggingOut" />
                        <ILogOut v-else />
                        {{ $t('sidebar.logout') }}
                    </UiDropdownMenuItem>
                </UiDropdownMenuContent>
            </UiDropdownMenu>
        </UiSidebarFooter>
    </UiSidebar>
</template>

<script setup lang="ts">
import { upperFirst } from 'scule';

import {
    IBookmark,
    IChartNoAxesColumn,
    IClipboardList,
    IFiles,
    IFolder,
    IHome,
    ILink,
    IMonitor,
    IMoon,
    IServer,
    ISun,
    ITrash2,
    IUsers,
} from '#components';
import { useSidebar } from '#layers/ui/app/components/ui/sidebar';

const authUser = useAuthUser();
const currentTheme = useTheme();
const colorMode = useColorMode();
const route = useRoute();
const { $toast } = useNuxtApp();
const { t, locale } = useI18n();
const { setOpenMobile } = useSidebar();
const trash = useTrash();
const storageUsage = useStorageUsage();
const isStorageWarned = useSessionStorage('fileship-storage-warned', false);
const isLoggingOut = ref(false);

const percentFormat = computed(
    () => new Intl.NumberFormat(locale.value, { style: 'percent', maximumFractionDigits: 0 }),
);

const storageRatio = computed(() =>
    storageUsage.value?.limit
        ? Number(storageUsage.value.used) / Number(storageUsage.value.limit)
        : null,
);

watch(storageRatio, (ratio) => {
    if (ratio === null || ratio < 0.9 || isStorageWarned.value) return;

    isStorageWarned.value = true;
    $toast.warning(t('storage.almostFull'), {
        description: t('storage.almostFullDescription'),
        action: {
            label: t('storage.manage'),
            onClick: () => navigateTo('/dashboard/settings/storage'),
        },
    });
});

const userInitials = computed(() => authUser.value!.username.slice(0, 2).toUpperCase());

const roleLabel = computed(() =>
    t(
        authUser.value!.superAdmin
            ? 'sidebar.superAdmin'
            : isAdmin(authUser.value)
              ? 'sidebar.admin'
              : 'sidebar.member',
    ),
);

const updaterStatus = ref<{
    version: string;
    url: string;
    hasUpdate: boolean;
    latestVersion: string;
    updaterAvailable: boolean;
} | null>(null);

const canUpdate = computed(
    () => !!updaterStatus.value?.updaterAvailable && !!authUser.value?.superAdmin,
);

const isUpdating = ref(false);
const updateState = ref('Pulling');

const updateStates = ['Pulling', 'Starting', 'Switching', 'Success', 'Error'];

const finishUpdate = (success: boolean) => {
    updateState.value = success ? 'Success' : 'Error';

    if (success) {
        $toast.success(t('sidebar.updated'));
        setTimeout(() => window.location.reload(), 2_000);
    } else {
        $toast.error(t('sidebar.updateFailed'));
    }
};

const waitForNewVersion = async () => {
    for (let attempt = 0; attempt < 90; attempt++) {
        const data = await $fetch('/api/healthz', { cache: 'no-store' }).catch(() => null);
        if (data?.version === updaterStatus.value!.latestVersion) return finishUpdate(true);

        await new Promise((resolve) => setTimeout(resolve, 2_000));
    }

    finishUpdate(false);
};

const handleUpdate = async () => {
    const confirmed = await askConfirmation({
        title: t('sidebar.updateConfirmTitle', { version: updaterStatus.value!.latestVersion }),
        description: t('sidebar.updateConfirmDescription'),
        confirmText: t('sidebar.update'),
    });

    if (!confirmed) return;

    updateState.value = 'Pulling';

    let isFinished = false;

    try {
        const response = await withVerification((verificationData) =>
            $fetch('/api/updater/update', {
                method: 'POST',
                body: { verificationData },
                responseType: 'stream',
                onResponseError: async ({ response }) => {
                    response._data = await new Response(response._data).json().catch(() => null);
                },
            }),
        );

        if (!response) return;

        isUpdating.value = true;

        await parseSseStream<{ status?: string }>(response, (data) => {
            if (!data.status) return;

            if (data.status === 'Success' || data.status === 'Error') isFinished = true;

            if (data.status === 'Success') waitForNewVersion();
            else if (data.status === 'Error') finishUpdate(false);
            else updateState.value = data.status;
        });
    } catch (error: any) {
        if (updateState.value === 'Switching') return waitForNewVersion();

        $toast.error(getErrorMessage(error));
        isUpdating.value = false;
        return;
    }

    if (isFinished) return;
    if (updateState.value === 'Switching') return waitForNewVersion();

    finishUpdate(false);
};

const colorModes = [
    { label: 'sidebar.light', value: 'light', icon: ISun },
    { label: 'sidebar.dark', value: 'dark', icon: IMoon },
    { label: 'sidebar.system', value: 'system', icon: IMonitor },
];

watch(
    () => route.fullPath,
    () => setOpenMobile(false),
);

const isActive = (url: string) =>
    route.path === url || (url !== '/dashboard' && route.path.startsWith(`${url}/`));

const items = computed(() => {
    const categories = [
        {
            title: t('sidebar.mainMenu'),
            children: [{ title: t('sidebar.home'), url: '/dashboard', icon: IHome }],
        },
    ];

    if (canUploadFiles(authUser.value)) {
        categories[0]!.children.push(
            { title: t('sidebar.files'), url: '/dashboard/files', icon: IFiles },
            { title: t('sidebar.folders'), url: '/dashboard/folders', icon: IFolder },
        );
    }

    if (canTakeNotes(authUser.value)) {
        categories[0]!.children.push({
            title: t('sidebar.notes'),
            url: '/dashboard/notes',
            icon: IBookmark,
        });
    }

    if (canShortenLinks(authUser.value)) {
        categories[0]!.children.push({
            title: t('sidebar.links'),
            url: '/dashboard/links',
            icon: ILink,
        });
    }

    if (trash.isEnabled.value && (canUploadFiles(authUser.value) || canTakeNotes(authUser.value))) {
        categories[0]!.children.push({
            title: t('sidebar.trash'),
            url: '/dashboard/trash',
            icon: ITrash2,
        });
    }

    if (isAdmin(authUser.value)) {
        categories.push({
            title: t('sidebar.administration'),
            children: [
                {
                    title: t('sidebar.statistics'),
                    url: '/dashboard/stats',
                    icon: IChartNoAxesColumn,
                },
                { title: t('sidebar.users'), url: '/dashboard/users', icon: IUsers },
                { title: t('sidebar.logs'), url: '/dashboard/logs', icon: IClipboardList },
                ...(authUser.value!.superAdmin
                    ? [{ title: t('sidebar.instance'), url: '/dashboard/instance', icon: IServer }]
                    : []),
            ],
        });
    }

    return categories;
});

const handleLanguageChange = (value: any) =>
    updateLanguage(value === 'system' ? null : (value as Language));

const handleLogout = async () => {
    isLoggingOut.value = true;

    try {
        await $fetch('/api/auth/logout', { method: 'POST' });

        clearStates(true, true);

        $toast.success(t('sidebar.loggedOut'));
        await navigateTo('/login');
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    isLoggingOut.value = false;
};

onMounted(async () => {
    if (isAdmin(authUser.value)) {
        try {
            updaterStatus.value = await $fetch<never>('/api/updater/status');
        } catch {
            //
        }
    }
});
</script>
