<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('sessions.title') }}</UiCardTitle>
            <UiCardDescription>{{ $t('sessions.description') }}</UiCardDescription>
            <UiCardAction>
                <UiButton variant="outline" :disabled="total < 2" @click="handleRevokeAll">
                    <ILogOut />
                    <span class="hidden sm:inline">{{ $t('sessions.signOutOthers') }}</span>
                </UiButton>
            </UiCardAction>
        </UiCardHeader>
        <UiCardContent>
            <div v-if="isLoading" class="flex flex-col gap-2">
                <ListCardSkeleton v-for="i in 3" :key="i" media="size-5" />
            </div>
            <EmptyState
                v-else-if="isError"
                :icon="ICircleAlert"
                :title="$t('sessions.loadFailed')"
                :description="$t('sessions.loadFailedDescription')"
            >
                <UiButton variant="outline" @click="refresh">
                    {{ $t('sessions.tryAgain') }}
                </UiButton>
            </EmptyState>
            <VirtualList
                v-else
                :items="sortedSessions"
                :item-key="(s) => s.id"
                :estimate-size="66"
                :loading="isLoadingMore"
                @reach-end="loadMore"
            >
                <template #skeleton>
                    <ListCardSkeleton media="size-5" />
                </template>
                <template #default="{ item }">
                    <UiCard size="sm">
                        <UiCardContent class="flex min-h-8 items-center gap-3">
                            <ISmartphone
                                v-if="
                                    item.os === 'Android' ||
                                    item.os === 'iOS' ||
                                    item.os === 'Mobile'
                                "
                                class="size-5 shrink-0 text-muted-foreground"
                            />
                            <IMonitor v-else class="size-5 shrink-0 text-muted-foreground" />
                            <div class="flex min-w-0 flex-1 flex-col">
                                <div class="flex items-center gap-2">
                                    <TextBlock variant="small" class="truncate">
                                        {{ deviceLabel(item.os) }} · {{ deviceLabel(item.platform) }}
                                    </TextBlock>
                                    <UiBadge
                                        v-if="item.id === currentUser!.currentSessionId"
                                        variant="secondary"
                                    >
                                        {{ $t('sessions.thisDevice') }}
                                    </UiBadge>
                                </div>
                                <TextBlock variant="muted" class="truncate text-xs">
                                    {{ item.ip }} · {{ locationLabel(item.location) }} ·
                                    {{ date(item.lastSeen).fromNow() }}
                                </TextBlock>
                            </div>
                            <UiButton
                                v-if="item.id !== currentUser!.currentSessionId"
                                variant="outline"
                                size="sm"
                                :disabled="revokingId === item.id"
                                @click="handleRevoke(item)"
                            >
                                <UiSpinner v-if="revokingId === item.id" />
                                <ILogOut v-else />
                                {{ $t('sessions.signOut') }}
                            </UiButton>
                        </UiCardContent>
                    </UiCard>
                </template>
            </VirtualList>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
import { ICircleAlert } from '#components';

import type { SessionData } from '~/types/sessions';

const currentUser = useAuthUser();
const { $toast } = useNuxtApp();
const { t, te } = useI18n();
const date = useDate();
const revokingId = ref<string | null>(null);

const deviceLabel = (value: string) =>
    te(`sessions.device.${value}`) ? t(`sessions.device.${value}`) : value;

const locationLabel = (value: string) =>
    value
        .split(', ')
        .map((part) => (part === 'Unknown' ? t('sessions.device.Unknown') : part))
        .join(', ');

const {
    items: sessions,
    total,
    isLoading,
    isError,
    isLoadingMore,
    loadMore,
    refresh,
} = usePaginatedList<SessionData>('sessions', '/api/users/@me/sessions');

const sortedSessions = computed(() =>
    [...sessions.value].sort((a, b) => {
        const currentId = currentUser.value!.currentSessionId;
        if (a.id === currentId) return -1;
        if (b.id === currentId) return 1;
        return +b.lastSeen - +a.lastSeen;
    }),
);

const handleRevoke = async (session: SessionData) => {
    revokingId.value = session.id;

    try {
        const result = await withVerification((verificationData) =>
            $fetch(`/api/users/@me/sessions/${session.id}`, {
                method: 'DELETE',
                body: { verificationData },
            }),
        );

        if (result) $toast.success(t('sessions.signedOut'));
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    revokingId.value = null;
};

const handleRevokeAll = async () => {
    await askConfirmation({
        title: t('sessions.signOutOthersTitle'),
        description: t('sessions.signOutOthersDescription'),
        confirmText: t('sessions.signOut'),
        action: async () => {
            try {
                const result = await withVerification((verificationData) =>
                    $fetch('/api/users/@me/sessions', {
                        method: 'DELETE',
                        body: { verificationData },
                    }),
                );

                if (result) $toast.success(t('sessions.othersSignedOut'));
            } catch (error: any) {
                $toast.error(getErrorMessage(error));
            }
        },
    });
};
</script>
