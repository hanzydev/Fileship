<template>
    <nav
        flex="~ items-center justify-between"
        z30
        h70px
        wfull
        bg-fs-overlay-2
        pl3
        pr5
        lg:px5
    >
        <svg
            :class="sidebarOpen && 'rotate-180'"
            cursor-pointer
            lg:hidden
            motion-safe:transition-transform
            viewBox="0 0 100 100"
            width="42"
            aria-label="Toggle sidebar"
            @click="sidebarOpen = !sidebarOpen"
        >
            <path
                stroke="white 5.5"
                fill-none
                motion-safe:transition-all
                :class="[
                    '[stroke-linecap:round]',
                    {
                        'stroke-dash-[40_82]': !sidebarOpen,
                        'stroke-dash-[14_82]': sidebarOpen,
                        '-stroke-offset-72': sidebarOpen,
                    },
                ]"
                d="m 30,33 h 40 c 0,0 8.5,-0.68551 8.5,10.375 0,8.292653 -6.122707,9.002293 -8.5,6.625 l -11.071429,-11.071429"
            />
            <path
                stroke="white 5.5"
                fill-none
                motion-safe:transition-all
                class="[stroke-linecap:round]"
                d="m 70,50 h -40"
            />
            <path
                stroke="white 5.5"
                fill-none
                motion-safe:transition-all
                :class="[
                    '[stroke-linecap:round]',
                    {
                        'stroke-dash-[40_82]': !sidebarOpen,
                        'stroke-dash-[14_82]': sidebarOpen,
                        '-stroke-offset-72': sidebarOpen,
                    },
                ]"
                d="m 30,67 h 40 c 0,0 8.5,0.68551 8.5,-10.375 0,-8.292653 -6.122707,-9.002293 -8.5,-6.625 l -11.071429,11.071429"
            />
        </svg>
        <div flex="~ gap2 items-center lg:items-end">
            <UiButton
                v-if="route.path.startsWith('/admin')"
                p0="!"
                h10
                w10
                alignment="center"
                icon="heroicons-solid:chevron-left"
                icon-size="32"
                aria-label="Go back to dashboard"
                href="/dashboard"
            />
            <h2 lt-lg:mxa lt-lg:text-2xl="!">
                {{ appConfig.site.name }}
            </h2>
            <UiButton
                v-if="latestRelease"
                variant="outline"
                alignment="center"
                p="y-0! x-1.5!"
                rounded="!"
                bg-fs-overlay-2
                ring-1
                :class="[
                    latestRelease.tag_name === `v${pkg.version}`
                        ? 'ring-fs-accent'
                        : 'ring-red-500 hover:!bg-red-500',
                ]"
                :href="`https://github.com/${repoUrl}/releases/tag/v${pkg.version}`"
                target="_blank"
            >
                v{{ pkg.version }}
            </UiButton>
        </div>
        <div lg:mla>
            <UiDropdown placement="bottom" right-0>
                <UiAvatar
                    :src="currentUser!.avatar || undefined"
                    :alt="currentUser!.username"
                    size="sm"
                    hover="ring-1 ring-fs-accent"
                    cursor-pointer
                    active:scale-95
                    motion-safe:transition-all
                />
                <template #content>
                    <div
                        w60
                        rounded-lg
                        bg-fs-overlay-2
                        p1.5
                        ring="1 fs-accent"
                        space-y-2
                    >
                        <div mx3.5 mt3.5 space-y-2>
                            <h4>
                                {{ upperFirst(currentUser!.username) }}
                            </h4>

                            <UiDivider />
                        </div>

                        <div space-y-1>
                            <UiButton
                                v-for="item in profileItems"
                                :key="item.name"
                                :variant="
                                    route.path.startsWith(item.href)
                                        ? 'accent'
                                        : 'primary'
                                "
                                wfull
                                gap2.5
                                icon-size="20"
                                :icon="item.icon"
                                :href="item.href"
                            >
                                {{ item.name }}
                            </UiButton>

                            <UiDropdown placement="left" hover>
                                <UiButton
                                    wfull
                                    gap2.5
                                    icon-size="20"
                                    icon="heroicons-solid:color-swatch"
                                >
                                    Themes
                                </UiButton>

                                <template #content>
                                    <div
                                        ring="1 fs-accent"
                                        h96
                                        w60
                                        overflow-y-auto
                                        rounded-lg
                                        bg-fs-overlay-2
                                        p1.5
                                        space-y-1
                                    >
                                        <UiButton
                                            v-for="[
                                                theme,
                                                { accent },
                                            ] in Object.entries(themes)"
                                            :key="theme"
                                            :variant="
                                                currentTheme === theme
                                                    ? 'accent'
                                                    : 'primary'
                                            "
                                            :icon="
                                                currentTheme === theme
                                                    ? 'heroicons-solid:check'
                                                    : undefined
                                            "
                                            wfull
                                            gap2.5
                                            icon-size="20"
                                            @click="
                                                currentTheme = theme as never
                                            "
                                        >
                                            <div
                                                v-if="currentTheme !== theme"
                                                h5
                                                w5
                                                rounded-full
                                                :style="{
                                                    backgroundColor: accent,
                                                }"
                                            ></div>
                                            {{ titleCase(theme) }}
                                        </UiButton>
                                    </div>
                                </template>
                            </UiDropdown>

                            <UiButton
                                wfull
                                gap2.5
                                text-red-500
                                icon-size="20"
                                icon="heroicons-solid:logout"
                                :disabled="isLoggingOut"
                                @click="handleLogout"
                            >
                                Logout
                            </UiButton>
                        </div>
                    </div>
                </template>
            </UiDropdown>
        </div>
    </nav>
</template>

<script setup lang="ts">
import { titleCase, upperFirst } from 'scule';
import { toast } from 'vue-sonner';

import themes from '~/styles/themes.json';
import pkg from '~~/package.json';
import { isAdmin } from '~~/utils/permissions';

const repoUrl = 'hanzydev/Fileship';

const { data: latestRelease } = await useFetch<any>(
    `https://api.github.com/repos/${repoUrl}/releases/latest`,
);

const route = useRoute();
const appConfig = useAppConfig();
const sidebarOpen = useSidebar();
const currentUser = useAuthUser();
const currentTheme = useTheme();

const isLoggingOut = ref(false);

const profileItems = computed(() => {
    const filtered = [
        {
            name: 'My Account',
            icon: 'iconamoon:profile-fill',
            href: '/dashboard/account',
        },

        {
            name: 'Sessions',
            icon: 'heroicons-solid:desktop-computer',
            href: '/dashboard/sessions',
        },
        {
            name: 'Backups',
            icon: 'heroicons-solid:archive',
            href: '/dashboard/backups',
        },
    ];

    if (isAdmin(currentUser.value!)) {
        filtered.push({
            name: 'Admin',
            icon: 'heroicons-solid:cog',
            href: '/admin',
        });
    }

    return filtered;
});

const handleLogout = async () => {
    isLoggingOut.value = true;

    useCookie('adminSessionId').value = null;
    await $fetch('/api/auth/logout', { method: 'POST' });

    toast.success('Logged out successfully');
};
</script>
