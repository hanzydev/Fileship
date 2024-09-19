<template>
    <div>
        <div v-if="currentUser" bg-fs-overlay-2>
            <LayoutsImpersonatingUser />
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
import type { IEmbed } from '~~/utils/types';
import {
    canShareCodes,
    canShortenUrls,
    canTakeNotes,
    canUploadFiles,
} from '~~/utils/user';

const embed = useEmbed();
const domains = useDomains();
const currentUser = useAuthUser();

const sidebarItems = computed(() => {
    const filtered = [
        {
            name: 'Home',
            icon: 'heroicons-solid:home',
            href: '/dashboard',
        },
    ];

    if (canUploadFiles(currentUser)) {
        filtered.push(
            {
                name: 'Files',
                icon: 'heroicons-solid:document-duplicate',
                href: '/dashboard/files',
            },
            {
                name: 'Folders',
                icon: 'heroicons-solid:folder',
                href: '/dashboard/folders',
            },
        );
    }

    if (canTakeNotes(currentUser)) {
        filtered.push({
            name: 'Notes',
            icon: 'heroicons:bookmark-solid',
            href: '/dashboard/notes',
        });
    }

    if (canShareCodes(currentUser)) {
        filtered.push({
            name: 'Codes',
            icon: 'heroicons-solid:code',
            href: '/dashboard/codes',
        });
    }

    if (canShortenUrls(currentUser)) {
        filtered.push({
            name: 'URLs',
            icon: 'heroicons:link-16-solid',
            href: '/dashboard/urls',
        });
    }

    return filtered;
});

onMounted(async () => {
    embed.value = await $fetch<IEmbed>('/api/users/@me/embed');
    domains.value = await $fetch<string[]>('/api/users/@me/domains');
});
</script>
