<template>
    <div>
        <div v-if="isAdmin(currentUser)" bg-fs-overlay-2>
            <LayoutsActingAsUser />
            <LayoutsNavbar />
            <LayoutsSidebar :items="sidebarItems" />
            <LayoutsDashboard>
                <slot />
            </LayoutsDashboard>
            <LayoutsUploadingFiles />
        </div>
    </div>
</template>

<script setup lang="ts">
const currentUser = useAuthUser();

const sidebarItems = [
    {
        name: 'Home',
        icon: 'heroicons-solid:home',
        href: '/admin',
    },
    {
        name: 'Users',
        icon: 'heroicons-solid:user-group',
        href: '/admin/users',
    },
    {
        name: 'Logs',
        icon: 'heroicons-solid:document-search',
        href: '/admin/logs',
    },
];
</script>
