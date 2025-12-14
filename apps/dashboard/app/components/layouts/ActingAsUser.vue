<template>
    <div
        v-if="adminSessionId"
        flex="~ items-center justify-center"
        m-6
        mb-0
        h-12
        rounded-2xl
        bg-fs-accent
        px2
        text-center
        border="b b-fs-background"
    >
        <UiButton
            p0="!"
            size-8
            rounded-xl="!"
            hover:bg="white/20"
            motion-safe:transition-colors
            alignment="center"
            icon="solar:arrow-left-linear"
            icon-size="20"
            variant="accent"
            :aria-label="`Stop acting as ${titleCase(currentUser!.username)}`"
            :loading="isReturningBack"
            :disabled="isReturningBack"
            @click="goBackAdminSession"
        />
        <p mx-auto font-medium>
            You're currently acting as
            {{ titleCase(currentUser!.username) }}.
        </p>
        <UiButton
            p0="!"
            size-8
            rounded-xl="!"
            hover:bg="white/20"
            motion-safe:transition-colors
            alignment="center"
            icon="lucide:x"
            icon-size="20"
            variant="accent"
            :aria-label="`Always act as ${titleCase(currentUser!.username)}`"
            :disabled="isReturningBack"
            @click="adminSessionId = null"
        />
    </div>
</template>

<script setup lang="ts">
import { titleCase } from 'scule';

const currentUser = useAuthUser();
const currentTheme = useTheme();
const adminSessionId = useCookie('adminSessionId');

const isReturningBack = ref(false);

const goBackAdminSession = async () => {
    isReturningBack.value = true;

    closeSocket();

    await $fetch('/api/auth/logout', { method: 'POST' });

    const user = await $fetch<AuthUserData>('/api/users/@me', {
        headers: {
            Authorization: adminSessionId.value!,
        },
    });

    currentUser.value = {
        ...user,
        createdAt: new Date(user.createdAt),
    };

    currentTheme.value = user.theme as never;

    useCookie('sessionId', {
        expires: new Date(Date.now() + 1000 * 60 * 60 * 365),
        path: '/',
        sameSite: 'lax',
    }).value = adminSessionId.value!;
    await nextTick();

    adminSessionId.value = null;

    await refreshNuxtData();
    await navigateTo('/dashboard/users');

    clearStates(true);
    initSocket();

    isReturningBack.value = false;
};
</script>
