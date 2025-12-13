<template>
    <div>
        <Transition
            enter-active-class="motion-safe:(animate-in fade-in)"
            leave-active-class="motion-safe:(animate-out fade-out)"
        >
            <div
                v-if="isOpen && width > 640 && width < 1024"
                absolute
                inset-0
                z20
                backdrop-blur-sm
                lg:ml64
                bg="black/60"
                :style="{
                    height: `calc(100vh - 70px${adminSessionId ? ' - 3rem' : ''})`,
                    marginTop: `calc(70px${adminSessionId ? ' + 48px' : ''})`,
                }"
                @click="isOpen = false"
            />
        </Transition>
        <Transition
            enter-active-class="motion-safe:(animate-in fade-in slide-in-left-full)"
            leave-active-class="motion-safe:(animate-out fade-out slide-out-left-full)"
        >
            <aside
                v-if="width < 1024 ? isOpen : true"
                :class="width === Infinity && 'lt-lg:hidden'"
                :style="{
                    height: `calc(100vh - 3rem${adminSessionId ? ' - 72px' : ''})`,
                }"
                motion-safe="transition-height"
                border="~ fs-overlay-3"
                data-allow-mismatch
                z20
                ml6
                mt6
                rounded-2xl
                bg-fs-overlay-1
                py6
                sm:w64
                space-y-6
            >
                <div flex="~ items-start gap-4" mx5 px2.5 lg:mx3>
                    <img
                        src="/fileship-192x192.png"
                        alt="fileship logo"
                        size-16
                        select-none
                        rounded-lg
                        draggable="false"
                    />
                    <div space-y-1>
                        <h3>Fileship</h3>
                        <Transition
                            enter-active-class="motion-safe:(animate-in fade-in zoom-in-95 slide-in-left-2)"
                        >
                            <UiButton
                                v-if="version"
                                variant="outline"
                                alignment="center"
                                p="y-0! x-2!"
                                rounded-xl="!"
                                wfit
                                bg-fs-overlay-2
                                ring-1
                                :class="[
                                    version.latest
                                        ? 'ring-fs-accent'
                                        : 'ring-red-500 hover:!bg-red-500',
                                ]"
                                :href="version.url"
                                target="_blank"
                            >
                                v{{ version.number }}
                            </UiButton>
                        </Transition>
                    </div>
                </div>
                <div v-for="category in categories" :key="category.name" space-y-2>
                    <span mx5 px2.5 text-sm text-fs-muted-3 font-bold lg:mx3>
                        {{ category.name }}
                    </span>
                    <div space-y-1.5>
                        <UiButton
                            v-for="item in category.items"
                            :key="item.name"
                            :href="item.href"
                            :variant="item.href === route.path ? 'accent' : 'ghost'"
                            :class="item.href !== route.path && 'hover:bg-fs-overlay-2'"
                            :icon="item.icon"
                            icon-size="20"
                            alignment="left"
                            rounded-xl="!"
                            mx5
                            gap2.5
                            px2.5
                            lg:mx3
                        >
                            {{ item.name }}
                        </UiButton>
                        <template v-if="category.name === 'ACCOUNT SETTINGS'">
                            <UiDropdown hover>
                                <UiButton
                                    icon-size="20"
                                    alignment="left"
                                    icon="solar:palette-bold"
                                    variant="ghost"
                                    hover="bg-fs-overlay-2"
                                    rounded-xl="!"
                                    mx5
                                    gap2.5
                                    px2.5
                                    lg:mx3
                                    w="[calc(100%-0.625rem-1.25rem)] lg:[calc(100%-0.625rem-0.75rem)]"
                                >
                                    Themes
                                </UiButton>

                                <template #content>
                                    <div
                                        ring="1 fs-overlay-4"
                                        h96
                                        w66
                                        overflow-y-auto
                                        rounded-xl
                                        bg-fs-overlay-2
                                        p1.5
                                        space-y-1
                                    >
                                        <UiButton
                                            v-for="[theme, { accent }] in Object.entries(themes)"
                                            :key="theme"
                                            :variant="
                                                currentTheme === theme ? 'accent' : 'onOverlay'
                                            "
                                            :icon="
                                                currentTheme === theme
                                                    ? 'solar:check-read-bold'
                                                    : undefined
                                            "
                                            wfull
                                            gap2.5
                                            icon-size="20"
                                            :disabled="toBeSyncedTheme"
                                            :loading="toBeSyncedTheme === theme"
                                            @click="syncTheme(theme as never)"
                                        >
                                            <div
                                                v-if="
                                                    currentTheme !== theme &&
                                                    toBeSyncedTheme !== theme
                                                "
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
                                variant="ghost"
                                icon-size="20"
                                alignment="left"
                                rounded-xl="!"
                                mx5
                                gap2.5
                                px2.5
                                lg:mx3
                                w="[calc(100%-0.625rem-1.25rem)] lg:[calc(100%-0.625rem-0.75rem)]"
                                hover="bg-red-500"
                                icon="solar:logout-2-bold"
                                :disabled="isLoggingOut"
                                :loading="isLoggingOut"
                                @click="handleLogout"
                            >
                                Logout
                            </UiButton>
                        </template>
                    </div>
                </div>
            </aside>
        </Transition>
    </div>
</template>

<script setup lang="ts">
import { titleCase } from 'scule';

import themes from '~/styles/themes.json';

const isOpen = useSidebar();
const router = useRouter();
const route = useRoute();
const overflow = useOverflow();
const currentTheme = useTheme();
const currentUser = useAuthUser();
const { width } = useWindowSize();
const { $toast } = useNuxtApp();

const adminSessionId = useCookie('adminSessionId');

const isLoggingOut = ref(false);
const toBeSyncedTheme = ref<keyof typeof themes | null>(null);

const version = ref<{
    number: string;
    url: string;
    latest: boolean;
} | null>(null);

const categories = computed(() => {
    const base = [
        {
            name: 'MAIN MENU',
            items: [
                {
                    name: 'Home',
                    icon: 'solar:home-2-bold',
                    href: '/dashboard',
                },
            ],
        },
    ];

    if (canUploadFiles(currentUser)) {
        base[0]!.items.push(
            {
                name: 'Files',
                icon: 'solar:documents-bold',
                href: '/dashboard/files',
            },
            {
                name: 'Folders',
                icon: 'solar:folder-bold',
                href: '/dashboard/folders',
            },
        );
    }

    if (canTakeNotes(currentUser)) {
        base[0]!.items.push({
            name: 'Notes',
            icon: 'solar:bookmark-bold',
            href: '/dashboard/notes',
        });
    }

    if (isAdmin(currentUser)) {
        base.push({
            name: 'ADMINISTRATION',
            items: [
                {
                    name: 'Statistics',
                    icon: 'solar:chart-square-bold',
                    href: '/admin',
                },
                {
                    name: 'Users',
                    icon: 'solar:users-group-rounded-bold',
                    href: '/admin/users',
                },
                {
                    name: 'Logs',
                    icon: 'solar:clipboard-list-bold',
                    href: '/admin/logs',
                },
            ],
        });
    }

    base.push({
        name: 'ACCOUNT SETTINGS',
        items: [
            {
                name: 'My Account',
                icon: 'solar:user-bold',
                href: '/dashboard/account',
            },

            {
                name: 'Sessions',
                icon: 'solar:monitor-bold',
                href: '/dashboard/sessions',
            },
            {
                name: 'Backups',
                icon: 'solar:archive-bold',
                href: '/dashboard/backups',
            },
        ],
    });

    return base;
});

const handleLogout = async () => {
    isLoggingOut.value = true;

    useCookie('adminSessionId').value = null;
    await $fetch('/api/auth/logout', { method: 'POST' });

    $toast.success('Logged out successfully');
};

const syncTheme = async (theme: keyof typeof themes) => {
    if (currentTheme.value !== theme) {
        toBeSyncedTheme.value = theme;

        await $fetch(`/api/users/${currentUser.value!.id}`, {
            method: 'PATCH',
            body: { theme },
        });

        toBeSyncedTheme.value = null;
    }
};

onKeyStroke(
    'Escape',
    () => {
        if (isOpen.value) isOpen.value = false;
    },
    { eventName: 'keydown' },
);

onUnmounted(() => (overflow.value = true));

watch(isOpen, (value) => (overflow.value = !value));

watch(width, (value) => (overflow.value = value >= 1024 ? true : !isOpen.value));

router.beforeEach((_, __, next) => {
    if (isOpen.value) isOpen.value = false;
    next();
});

onMounted(async () => {
    try {
        version.value = await $fetch<never>('/api/version');
    } catch {
        //
    }
});
</script>
