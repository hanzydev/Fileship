<template>
    <div>
        <Transition
            enter-active-class="motion-safe:(animate-in fade-in slide-in-left-full animate-duration-300)"
            leave-active-class="motion-safe:(animate-out fade-out slide-out-left-full animate-duration-300)"
        >
            <aside
                v-if="width < 768 ? isOpen : true"
                :class="width === Infinity && 'lt-md:hidden'"
                :style="{
                    height: `calc(100vh - 3rem${adminSessionId ? ' - 72px' : ''})`,
                }"
                motion-safe="transition-height"
                border="~ fs-overlay-3"
                md="w64"
                lt-md="absolute w-[calc(100%-48px)]!"
                data-allow-mismatch
                relative
                z20
                ml6
                mt6
                overflow-hidden
                rounded-2xl
                bg-fs-overlay-1
            >
                <Transition
                    enter-active-class="motion-safe:(animate-in fade-in data-[view=main]:slide-in-from-left data-[view=themes]:slide-in-from-right animate-duration-250)"
                    leave-active-class="motion-safe:(animate-out fade-out data-[view=main]:slide-out-to-left data-[view=themes]:slide-out-to-right animate-duration-250)"
                    :data-view="currentView"
                >
                    <div
                        v-if="currentView === 'main'"
                        data-view="main"
                        flex="~ col"
                        absolute
                        inset-0
                        hfull
                        wfull
                        pb2.5
                        pt5
                    >
                        <div flex="~ items-center gap-3" mx-5 mb-6 flex-shrink-0>
                            <img
                                src="/fileship.png"
                                alt="fileship"
                                class="size-10"
                                draggable="false"
                            />
                            <h3 text-lg font-bold>Fileship</h3>

                            <UiButton
                                alignment="center"
                                icon="lucide:x"
                                icon-size="24"
                                p0="!"
                                relative
                                mla
                                size-10
                                flex-shrink-0
                                gap-2
                                md="hidden"
                                rounded-xl="!"
                                @click="isOpen = false"
                            />
                        </div>

                        <div flex-1 overflow-y-auto space-y-6>
                            <div v-for="category in categories" :key="category.name" space-y-2>
                                <span mx2.5 px2.5 text-sm text-fs-muted-3 font-bold>
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
                                        mx2.5
                                        gap2.5
                                        px2.5
                                    >
                                        {{ item.name }}
                                    </UiButton>

                                    <template v-if="category.name === 'ACCOUNT SETTINGS'">
                                        <UiButton
                                            icon-size="20"
                                            alignment="left"
                                            icon="solar:palette-bold"
                                            variant="ghost"
                                            hover="bg-fs-overlay-2"
                                            rounded-xl="!"
                                            mx2.5
                                            gap2.5
                                            px2.5
                                            w="[calc(100%-1.25rem)]"
                                            @click="currentView = 'themes'"
                                        >
                                            Themes
                                        </UiButton>

                                        <UiButton
                                            variant="ghost"
                                            icon-size="20"
                                            alignment="left"
                                            rounded-xl="!"
                                            mx2.5
                                            gap2.5
                                            px2.5
                                            w="[calc(100%-1.25rem)]"
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
                        </div>

                        <div v-if="version" mt-auto px2.5 pt2>
                            <a
                                :href="version.url"
                                target="_blank"
                                flex="~ items-center justify-between"
                                w-full
                                rounded-xl
                                p-2.5
                                active="scale-95"
                                motion-safe="transition-all"
                                :class="
                                    version.latest
                                        ? 'bg-fs-overlay-2 hover:bg-fs-overlay-3 text-fs-muted-2'
                                        : 'bg-red-500/10 text-red-500'
                                "
                            >
                                <div flex="~ col gap-0.5">
                                    <span text-xs font-medium>Current Version</span>
                                    <span text-sm font-bold leading-none>
                                        v{{ version.number }}
                                    </span>
                                </div>

                                <div
                                    flex="~ items-center justify-center"
                                    size-8
                                    rounded-lg
                                    text-white
                                    :class="version.latest ? 'bg-fs-accent/10' : 'bg-red-500'"
                                >
                                    <Icon
                                        :name="
                                            version.latest
                                                ? 'solar:verified-check-bold'
                                                : 'solar:download-square-bold'
                                        "
                                        size="20"
                                    />
                                </div>
                            </a>
                        </div>
                    </div>
                    <div
                        v-else-if="currentView === 'themes'"
                        data-view="themes"
                        absolute
                        inset-0
                        hfull
                        wfull
                        overflow-y-auto
                        p5
                        space-y-6
                    >
                        <div flex="~ items-center gap-4">
                            <UiButton
                                icon="solar:arrow-left-linear"
                                icon-size="20"
                                alignment="center"
                                size-10
                                gap-2
                                rounded-xl="!"
                                @click="currentView = 'main'"
                            />
                            <h3>Themes</h3>
                        </div>
                        <div space-y-1>
                            <UiButton
                                v-for="[theme, { accent }] in Object.entries(themes)"
                                :key="theme"
                                :variant="currentTheme === theme ? 'accent' : 'ghost'"
                                :icon="currentTheme === theme ? 'lucide:check' : undefined"
                                :class="currentTheme !== theme && 'hover:bg-fs-overlay-2'"
                                rounded-xl="!"
                                wfull
                                gap2.5
                                px2.5
                                alignment="left"
                                icon-size="20"
                                :disabled="toBeSyncedTheme"
                                :loading="toBeSyncedTheme === theme"
                                @click="syncTheme(theme as never)"
                            >
                                <div
                                    v-if="currentTheme !== theme && toBeSyncedTheme !== theme"
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
                    </div>
                </Transition>
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
const currentTheme = useTheme();
const currentUser = useAuthUser();
const { width } = useWindowSize();
const { $toast } = useNuxtApp();

const adminSessionId = useCookie('adminSessionId');

const isLoggingOut = ref(false);
const currentView = ref<'main' | 'themes'>('main');
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
                    href: '/dashboard/stats',
                },
                {
                    name: 'Users',
                    icon: 'solar:users-group-rounded-bold',
                    href: '/dashboard/users',
                },
                {
                    name: 'Logs',
                    icon: 'solar:clipboard-list-bold',
                    href: '/dashboard/logs',
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
