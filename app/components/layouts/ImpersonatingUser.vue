<template>
    <div
        v-if="adminSessionId"
        h-12
        flex="~ items-center"
        bg-fs-accent
        pl3
        pr5
        lg:px5
        border="b b-fs-5"
    >
        <UiButton
            p0="!"
            h8
            w8
            rounded="!"
            hover:bg-white="/20"
            motion-safe:transition-colors
            alignment="center"
            icon="heroicons-solid:chevron-left"
            icon-size="24"
            variant="accent"
            aria-label="Stop impersonating the user"
            :loading="switching"
            :disabled="switching"
            @click="goBackAdminSession"
        />
        <p mx-auto font-medium>
            You're now impersonating user
            {{ titleCase(currentUser!.username) }}.
        </p>
        <UiButton
            p0="!"
            h8
            w8
            rounded="!"
            hover:bg-white="/20"
            motion-safe:transition-colors
            alignment="center"
            icon="heroicons-solid:x"
            icon-size="24"
            variant="accent"
            aria-label="Always impersonate the user"
            :loading="switching"
            :disabled="switching"
            @click="adminSessionId = null"
        />
    </div>
</template>

<script setup lang="ts">
import { titleCase } from 'scule';

const currentUser = useAuthUser();
const adminSessionId = useCookie('adminSessionId');

const switching = ref(false);

const goBackAdminSession = async () => {
    switching.value = true;

    closeSocket();

    await $fetch('/api/auth/logout', { method: 'POST' });

    const user = await $fetch('/api/users/@me', {
        headers: {
            Authorization: adminSessionId.value!,
        },
    });

    currentUser.value = {
        ...user,
        createdAt: new Date(user.createdAt),
    };

    useCookie('sessionId', {
        expires: new Date(Date.now() + 1000 * 60 * 60 * 365),
        path: '/',
        sameSite: true,
    }).value = adminSessionId.value!;
    await nextTick();

    adminSessionId.value = null;

    await refreshNuxtData();
    await navigateTo('/admin/users');

    initSocket();

    switching.value = false;
};
</script>
