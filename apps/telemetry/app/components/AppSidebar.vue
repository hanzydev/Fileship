<template>
    <UiSidebar collapsible="icon">
        <UiSidebarHeader>
            <UiSidebarMenu>
                <UiSidebarMenuItem>
                    <UiSidebarMenuButton
                        as-child
                        class="group-data-[collapsible=icon]:justify-center"
                        size="lg"
                        tooltip="Fileship Telemetry"
                    >
                        <NuxtLink to="/overview" class="flex items-center gap-4">
                            <img
                                src="/fileship.png"
                                alt=""
                                class="size-10 group-data-[collapsible=icon]:size-4"
                            />
                            <div class="flex flex-col group-data-[collapsible=icon]:hidden">
                                <TextBlock variant="large">Fileship</TextBlock>
                                <TextBlock variant="muted">Telemetry</TextBlock>
                            </div>
                        </NuxtLink>
                    </UiSidebarMenuButton>
                </UiSidebarMenuItem>
            </UiSidebarMenu>
        </UiSidebarHeader>
        <UiSidebarContent>
            <UiSidebarGroup>
                <UiSidebarGroupLabel>Telemetry</UiSidebarGroupLabel>
                <UiSidebarGroupContent>
                    <UiSidebarMenu>
                        <UiSidebarMenuItem v-for="item in items" :key="item.url">
                            <UiSidebarMenuButton
                                as-child
                                :is-active="isActive(item.url)"
                                :tooltip="item.title"
                            >
                                <NuxtLink :to="item.url">
                                    <component :is="item.icon" />
                                    <span>{{ item.title }}</span>
                                </NuxtLink>
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
                        tooltip="Logout"
                        :disabled="isLoggingOut"
                        @click="handleLogout"
                    >
                        <UiSpinner v-if="isLoggingOut" />
                        <ILogOut v-else />
                        <span>Logout</span>
                    </UiSidebarMenuButton>
                </UiSidebarMenuItem>
            </UiSidebarMenu>
        </UiSidebarFooter>
    </UiSidebar>
</template>

<script setup lang="ts">
import { IChartNoAxesCombined, IGitBranch, IHardDrive, IServer, ISparkles } from '#components';
import { useSidebar } from '#layers/ui/app/components/ui/sidebar';

const admin = useAdmin();
const route = useRoute();
const { $toast } = useNuxtApp();
const { setOpenMobile } = useSidebar();

const isLoggingOut = ref(false);

const items = [
    { title: 'Overview', url: '/overview', icon: IChartNoAxesCombined },
    { title: 'Versions', url: '/versions', icon: IGitBranch },
    { title: 'System', url: '/system', icon: IHardDrive },
    { title: 'Features', url: '/features', icon: ISparkles },
    { title: 'Instances', url: '/instances', icon: IServer },
];

const isActive = (url: string) => route.path === url || route.path.startsWith(`${url}/`);

const handleLogout = async () => {
    isLoggingOut.value = true;

    try {
        await $fetch('/api/auth/logout', { method: 'POST' });

        admin.value = false;
        await navigateTo('/login');
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    isLoggingOut.value = false;
};

watch(
    () => route.fullPath,
    () => setOpenMobile(false),
);
</script>
