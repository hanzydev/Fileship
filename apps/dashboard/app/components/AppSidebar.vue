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
                            updaterStatus ? `Fileship - v${updaterStatus?.version}` : 'Fileship'
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
                                        v{{ updaterStatus?.version }}
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
            <template v-if="updaterStatus?.hasUpdate">
                <UiCard size="sm" class="group-data-[collapsible=icon]:hidden">
                    <UiCardHeader>
                        <UiCardTitle class="flex items-center gap-2">
                            <ISparkles class="size-4 text-primary" />
                            Update available
                        </UiCardTitle>
                        <UiCardDescription>
                            v{{ updaterStatus.version }} → v{{ updaterStatus.latestVersion }}
                        </UiCardDescription>
                    </UiCardHeader>
                    <UiCardFooter class="gap-2">
                        <UiButton v-if="canUpdate" size="sm" class="flex-1" @click="handleUpdate">
                            Update
                        </UiButton>
                        <UiButton
                            size="sm"
                            :variant="canUpdate ? 'outline' : 'default'"
                            :class="!canUpdate && 'flex-1'"
                            as-child
                        >
                            <a :href="updaterStatus.url" target="_blank" rel="noopener noreferrer">
                                Changelog
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
                            <UiDialogTitle>Updating Fileship</UiDialogTitle>
                            <UiDialogDescription>
                                {{ updateStates[updateState] || updateState }}
                            </UiDialogDescription>
                        </UiDialogHeader>
                        <UiDialogFooter v-if="updateState === 'Error'">
                            <UiButton variant="outline" class="w-full" @click="isUpdating = false">
                                Close
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
                        tooltip="Account"
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
                            Settings
                        </NuxtLink>
                    </UiDropdownMenuItem>
                    <UiDropdownMenuSub>
                        <UiDropdownMenuSubTrigger>
                            <IPalette />
                            Appearance
                        </UiDropdownMenuSubTrigger>
                        <UiDropdownMenuSubContent>
                            <UiDropdownMenuLabel>Mode</UiDropdownMenuLabel>
                            <UiDropdownMenuRadioGroup v-model="colorMode.preference">
                                <UiDropdownMenuRadioItem
                                    v-for="mode in colorModes"
                                    :key="mode.value"
                                    :value="mode.value"
                                    @select.prevent
                                >
                                    <component :is="mode.icon" />
                                    {{ mode.label }}
                                </UiDropdownMenuRadioItem>
                            </UiDropdownMenuRadioGroup>
                            <UiDropdownMenuSeparator />
                            <UiDropdownMenuLabel>Accent</UiDropdownMenuLabel>
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
                                    {{ accent }}
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
                        Logout
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
const { setOpenMobile } = useSidebar();
const trash = useTrash();
const isLoggingOut = ref(false);

const userInitials = computed(() => authUser.value!.username.slice(0, 2).toUpperCase());

const roleLabel = computed(() =>
    authUser.value!.superAdmin ? 'Super admin' : isAdmin(authUser.value) ? 'Admin' : 'Member',
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

const updateStates: Record<string, string> = {
    Pulling: 'Downloading the new version...',
    Starting: 'Starting the new version next to the current one...',
    Switching: 'Moving traffic to the new version...',
    Success: 'Update completed, reloading...',
    Error: 'Update failed, the current version keeps running.',
};

const finishUpdate = (success: boolean) => {
    updateState.value = success ? 'Success' : 'Error';

    if (success) {
        $toast.success('Fileship updated successfully!');
        setTimeout(() => window.location.reload(), 2_000);
    } else {
        $toast.error('An unknown error occurred while updating Fileship');
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
        title: `Update to v${updaterStatus.value!.latestVersion}?`,
        description: 'The new version starts next to the current one, so Fileship stays online.',
        confirmText: 'Update',
    });

    if (!confirmed) return;

    isUpdating.value = true;
    updateState.value = 'Pulling';

    let isFinished = false;

    try {
        const response = await $fetch('/api/updater/update', {
            method: 'POST',
            responseType: 'stream',
        });

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
    { label: 'Light', value: 'light', icon: ISun },
    { label: 'Dark', value: 'dark', icon: IMoon },
    { label: 'System', value: 'system', icon: IMonitor },
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
            title: 'Main menu',
            children: [{ title: 'Home', url: '/dashboard', icon: IHome }],
        },
    ];

    if (canUploadFiles(authUser.value)) {
        categories[0]!.children.push(
            { title: 'Files', url: '/dashboard/files', icon: IFiles },
            { title: 'Folders', url: '/dashboard/folders', icon: IFolder },
        );
    }

    if (canTakeNotes(authUser.value)) {
        categories[0]!.children.push({ title: 'Notes', url: '/dashboard/notes', icon: IBookmark });
    }

    if (canShortenLinks(authUser.value)) {
        categories[0]!.children.push({ title: 'Links', url: '/dashboard/links', icon: ILink });
    }

    if (trash.isEnabled.value && (canUploadFiles(authUser.value) || canTakeNotes(authUser.value))) {
        categories[0]!.children.push({ title: 'Trash', url: '/dashboard/trash', icon: ITrash2 });
    }

    if (isAdmin(authUser.value)) {
        categories.push({
            title: 'Administration',
            children: [
                { title: 'Statistics', url: '/dashboard/stats', icon: IChartNoAxesColumn },
                { title: 'Users', url: '/dashboard/users', icon: IUsers },
                { title: 'Logs', url: '/dashboard/logs', icon: IClipboardList },
                ...(authUser.value!.superAdmin
                    ? [{ title: 'Instance', url: '/dashboard/instance', icon: IServer }]
                    : []),
            ],
        });
    }

    return categories;
});

const handleLogout = async () => {
    isLoggingOut.value = true;

    try {
        await $fetch('/api/auth/logout', { method: 'POST' });

        clearStates(true, true);

        $toast.success('Logged out successfully');
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
