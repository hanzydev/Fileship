<template>
    <div w="[calc(100%-48px)] md:[calc(100%-256px-48px-24px)]" m="t6 l6" space-y-6>
        <section
            flex="~ items-center justify-between gap-4"
            border="~ fs-overlay-3"
            h5rem
            rounded-2xl
            bg-fs-overlay-1
            px-6
        >
            <slot name="header" />

            <div>
                <UiButton
                    alignment="center"
                    icon="solar:hamburger-menu-linear"
                    icon-size="24"
                    p0="!"
                    relative
                    mla
                    size-10
                    flex-shrink-0
                    gap-2
                    md="hidden"
                    rounded-xl="!"
                    @click="sidebarOpen = true"
                />
                <UiAvatar
                    :src="currentUser!.avatar || undefined"
                    :alt="currentUser!.username"
                    size="sm"
                    lt-md="hidden"
                    cursor-pointer
                    active:scale-95
                    hover:ring-fs-accent
                    motion-safe:transition-all
                    @click="$router.push('/dashboard/account')"
                />
            </div>
        </section>
        <section
            border="~ fs-overlay-3"
            motion-safe="transition-max-height"
            wfull
            overflow-x-hidden
            rounded-2xl
            bg-fs-overlay-1
            p6
            space-y-6
            :style="{
                maxHeight: `calc(100vh - 3rem - 5rem - 24px${adminSessionId ? ' - 72px' : ''})`,
            }"
        >
            <slot />
        </section>
    </div>
</template>

<script setup lang="ts">
const adminSessionId = useCookie('adminSessionId');
const sidebarOpen = useSidebar();
const currentUser = useAuthUser();
</script>
