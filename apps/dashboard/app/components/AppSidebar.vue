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
                            type="button"
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
                <UiSidebarGroupLabel>
                    <TextBlock variant="small">{{ category.title }}</TextBlock>
                </UiSidebarGroupLabel>
                <UiSidebarGroupContent>
                    <UiSidebarMenu>
                        <UiSidebarMenuItem v-for="child in category.children" :key="child.title">
                            <UiSidebarMenuButton
                                as-child
                                :is-active="isActive(child.url)"
                                :tooltip="child.title"
                            >
                                <NuxtLink :to="child.url">
                                    <component :is="child.icon" :size="16" />
                                    <TextBlock variant="small">{{ child.title }}</TextBlock>
                                </NuxtLink>
                            </UiSidebarMenuButton>
                        </UiSidebarMenuItem>
                    </UiSidebarMenu>

                    <!-- TODO: UPDATER UI -->
                </UiSidebarGroupContent>
            </UiSidebarGroup>
        </UiSidebarContent>
        <UiSidebarFooter>
            <UiDropdownMenu>
                <UiDropdownMenuTrigger as-child>
                    <UiSidebarMenuButton
                        size="lg"
                        :is-active="isActive('/dashboard/account')"
                        tooltip="Account"
                    >
                        <UiAvatar>
                            <UiAvatarImage
                                v-if="authUser?.avatar"
                                :src="authUser.avatar"
                                :alt="authUser.username"
                            />
                            <UiAvatarFallback>
                                <TextBlock variant="small">{{ userInitials }}</TextBlock>
                            </UiAvatarFallback>
                        </UiAvatar>
                        <TextBlock variant="small" class="text-muted-foreground">
                            {{ upperFirst(authUser!.username) }}
                        </TextBlock>
                        <IChevronsUpDown class="ml-auto" :size="16" />
                    </UiSidebarMenuButton>
                </UiDropdownMenuTrigger>
                <UiDropdownMenuContent side="top">
                    <div class="flex items-center gap-2 px-3 py-2">
                        <UiAvatar>
                            <UiAvatarImage
                                v-if="authUser?.avatar"
                                :src="authUser.avatar"
                                :alt="authUser.username"
                            />
                            <UiAvatarFallback>
                                <TextBlock variant="small">{{ userInitials }}</TextBlock>
                            </UiAvatarFallback>
                        </UiAvatar>
                        <TextBlock variant="small">
                            {{ upperFirst(authUser!.username) }}
                        </TextBlock>
                    </div>
                    <UiDropdownMenuSeparator />
                    <UiDropdownMenuItem as-child>
                        <NuxtLink to="/dashboard/account">
                            <IUser :size="16" />
                            <TextBlock variant="small">My Account</TextBlock>
                        </NuxtLink>
                    </UiDropdownMenuItem>
                    <UiDropdownMenuItem as-child>
                        <NuxtLink to="/dashboard/sessions">
                            <IMonitor :size="16" />
                            <TextBlock variant="small">Sessions</TextBlock>
                        </NuxtLink>
                    </UiDropdownMenuItem>
                    <UiDropdownMenuItem as-child>
                        <NuxtLink to="/dashboard/backups">
                            <IArchive :size="16" />
                            <TextBlock variant="small">Backups</TextBlock>
                        </NuxtLink>
                    </UiDropdownMenuItem>
                    <UiDropdownMenuSeparator />
                    <UiDropdownMenuItem
                        variant="destructive"
                        :disabled="isLoggingOut"
                        @click="handleLogout"
                    >
                        <UiSpinner v-if="isLoggingOut" />
                        <ILogOut v-else :size="16" />
                        <TextBlock variant="small">Logout</TextBlock>
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
    IFileText,
    IFolder,
    IHome,
    IUsers,
} from '#components';

const authUser = useAuthUser();
const route = useRoute();
const { $toast } = useNuxtApp();
const isLoggingOut = ref(false);

const userInitials = computed(() => authUser.value!.username.slice(0, 2).toUpperCase());

const updaterStatus = ref<{
    version: string;
    url: string;
    hasUpdate: boolean;
    latestVersion: string;
    updaterAvailable: boolean;
} | null>(null);

const isActive = (url: string) => route.path === url || route.path.startsWith(`${url}/`);

const items = computed(() => {
    const categories = [
        {
            title: 'Main menu',
            children: [{ title: 'Home', url: '/dashboard', icon: IHome }],
        },
    ];

    if (canUploadFiles(authUser.value)) {
        categories[0]!.children.push(
            { title: 'Files', url: '/dashboard/files', icon: IFileText },
            { title: 'Folders', url: '/dashboard/folders', icon: IFolder },
        );
    }

    if (canTakeNotes(authUser.value)) {
        categories[0]!.children.push({ title: 'Notes', url: '/dashboard/notes', icon: IBookmark });
    }

    if (isAdmin(authUser.value)) {
        categories.push({
            title: 'Administration',
            children: [
                { title: 'Statistics', url: '/dashboard/stats', icon: IChartNoAxesColumn },
                { title: 'Users', url: '/dashboard/users', icon: IUsers },
                { title: 'Logs', url: '/dashboard/logs', icon: IClipboardList },
            ],
        });
    }

    return categories;
});

const handleLogout = async () => {
    isLoggingOut.value = true;
    useCookie('adminSessionId').value = null;

    try {
        await $fetch('/api/auth/logout', { method: 'POST' });
        $toast.success('Logged out successfully');
    } finally {
        isLoggingOut.value = false;
    }
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
