<template>
    <UiSidebar collapsible="icon">
        <UiSidebarHeader>
            <UiSidebarMenu>
                <UiSidebarMenuItem>
                    <UiSidebarMenuButton
                        as-child
                        class="group-data-[collapsible=icon]:justify-center"
                        size="lg"
                        :tooltip="account?.siteName ?? 'Fileship'"
                    >
                        <NuxtLink to="/home" class="flex items-center gap-4">
                            <img
                                src="/fileship.png"
                                :alt="''"
                                class="size-10 group-data-[collapsible=icon]:size-4"
                            />
                            <div class="flex flex-col group-data-[collapsible=icon]:hidden">
                                <TextBlock variant="large">Fileship</TextBlock>
                                <TextBlock variant="muted" class="truncate">
                                    {{
                                        account?.siteName === 'Fileship'
                                            ? $t('nav.forWindows')
                                            : account?.siteName
                                    }}
                                </TextBlock>
                            </div>
                        </NuxtLink>
                    </UiSidebarMenuButton>
                </UiSidebarMenuItem>
            </UiSidebarMenu>
        </UiSidebarHeader>
        <UiSidebarContent>
            <UiSidebarGroup>
                <UiSidebarGroupLabel>Fileship</UiSidebarGroupLabel>
                <UiSidebarGroupContent>
                    <UiSidebarMenu>
                        <UiSidebarMenuItem v-for="item in items" :key="item.url">
                            <UiSidebarMenuButton
                                as-child
                                :is-active="isActive(item.url)"
                                :tooltip="$t(item.title)"
                            >
                                <NuxtLink :to="item.url">
                                    <component :is="item.icon" />
                                    <span>{{ $t(item.title) }}</span>
                                </NuxtLink>
                            </UiSidebarMenuButton>
                        </UiSidebarMenuItem>
                        <UiSidebarMenuItem>
                            <UiSidebarMenuButton
                                :tooltip="$t('nav.dashboard')"
                                @click="handleOpenDashboard"
                            >
                                <IExternalLink />
                                <span>{{ $t('nav.dashboard') }}</span>
                            </UiSidebarMenuButton>
                        </UiSidebarMenuItem>
                    </UiSidebarMenu>
                </UiSidebarGroupContent>
            </UiSidebarGroup>
        </UiSidebarContent>
        <UiSidebarFooter>
            <UiSidebarMenu>
                <UiSidebarMenuItem>
                    <UiSidebarMenuButton
                        as-child
                        size="lg"
                        :is-active="isActive('/settings/account')"
                        :tooltip="$t('nav.account')"
                    >
                        <NuxtLink to="/settings/account">
                            <UiAvatar class="size-8">
                                <UiAvatarImage
                                    v-if="account?.user.avatar"
                                    :src="account.user.avatar"
                                    :alt="account.user.username"
                                />
                                <UiAvatarFallback>{{ initials }}</UiAvatarFallback>
                            </UiAvatar>
                            <div class="grid flex-1 text-left text-sm leading-tight">
                                <span class="truncate font-medium">
                                    {{ account?.user.username }}
                                </span>
                                <span class="truncate text-xs text-muted-foreground">
                                    {{ host }}
                                </span>
                            </div>
                        </NuxtLink>
                    </UiSidebarMenuButton>
                </UiSidebarMenuItem>
            </UiSidebarMenu>
        </UiSidebarFooter>
    </UiSidebar>
</template>

<script setup lang="ts">
import { IHome, ISettings } from '#components';
import { useSidebar } from '#layers/ui/app/components/ui/sidebar';

const account = useAccount();
const route = useRoute();
const { locale } = useI18n();
const { setOpenMobile } = useSidebar();

const items = [
    { title: 'nav.home', url: '/home', icon: IHome },
    { title: 'nav.settings', url: '/settings', icon: ISettings },
];

const initials = computed(
    () => account.value?.user.username.slice(0, 2).toLocaleUpperCase(locale.value) ?? '',
);

const host = computed(() => (account.value ? new URL(account.value.instanceUrl).host : ''));

const isActive = (url: string) => route.path === url || route.path.startsWith(`${url}/`);

const handleOpenDashboard = () => desktop('app:openDashboard');

watch(
    () => route.fullPath,
    () => setOpenMobile(false),
);
</script>
