<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>Sessions</UiCardTitle>
            <UiCardDescription>Devices that are signed in to your account.</UiCardDescription>
            <UiCardAction>
                <UiButton
                    variant="outline"
                    :disabled="total < 2 || isRevokingAll"
                    @click="handleRevokeAll"
                >
                    <UiSpinner v-if="isRevokingAll" />
                    <ILogOut v-else />
                    Sign out others
                </UiButton>
            </UiCardAction>
        </UiCardHeader>
        <UiCardContent>
            <div v-if="isLoading" class="flex flex-col gap-2">
                <UiSkeleton v-for="i in 3" :key="i" class="h-16 rounded-2xl" />
            </div>
            <VirtualList
                v-else
                :items="sortedSessions"
                :item-key="(s) => s.id"
                :estimate-size="66"
                :loading="isLoadingMore"
                @reach-end="loadMore"
            >
                <template #default="{ item }">
                    <div class="flex items-center gap-3 rounded-2xl border px-4 py-3">
                        <ISmartphone
                            v-if="
                                item.os === 'Android' || item.os === 'iOS' || item.os === 'Mobile'
                            "
                            class="size-5 shrink-0 text-muted-foreground"
                        />
                        <IMonitor v-else class="size-5 shrink-0 text-muted-foreground" />
                        <div class="flex min-w-0 flex-1 flex-col">
                            <div class="flex items-center gap-2">
                                <TextBlock variant="small" class="truncate">
                                    {{ item.os }} · {{ item.platform }}
                                </TextBlock>
                                <UiBadge v-if="item.id === currentUser!.currentSessionId">
                                    This device
                                </UiBadge>
                            </div>
                            <TextBlock variant="muted" class="truncate text-xs">
                                {{ item.ip }} · {{ item.location }} ·
                                {{ dayjs(item.lastSeen).fromNow() }}
                            </TextBlock>
                        </div>
                        <UiButton
                            v-if="item.id !== currentUser!.currentSessionId"
                            variant="ghost"
                            size="sm"
                            @click="handleRevoke(item)"
                        >
                            Sign out
                        </UiButton>
                    </div>
                </template>
            </VirtualList>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

const currentUser = useAuthUser();
const { $toast } = useNuxtApp();

const {
    items: sessions,
    total,
    isLoading,
    isLoadingMore,
    loadMore,
} = usePaginatedList<SessionData>('sessions', '/api/users/@me/sessions');

const isRevokingAll = ref(false);

const sortedSessions = computed(() =>
    [...sessions.value].sort((a, b) => {
        const currentId = currentUser.value!.currentSessionId;
        if (a.id === currentId) return -1;
        if (b.id === currentId) return 1;
        return +b.lastSeen - +a.lastSeen;
    }),
);

const handleRevoke = async (session: SessionData) => {
    try {
        const result = await withVerification((verificationData) =>
            $fetch(`/api/users/@me/sessions/${session.id}`, {
                method: 'DELETE',
                body: { verificationData },
            }),
        );

        if (result) $toast.success('Session signed out');
    } catch (error: any) {
        $toast.error(error.data.message);
    }
};

const handleRevokeAll = async () => {
    const confirmed = await askConfirmation({
        title: 'Sign out all other devices?',
        description: 'Every session except this one will be signed out.',
        confirmText: 'Sign out',
    });

    if (!confirmed) return;

    isRevokingAll.value = true;

    try {
        const result = await withVerification((verificationData) =>
            $fetch('/api/users/@me/sessions', {
                method: 'DELETE',
                body: { verificationData },
            }),
        );

        if (result) $toast.success('Other sessions signed out');
    } catch (error: any) {
        $toast.error(error.data.message);
    }

    isRevokingAll.value = false;
};
</script>
