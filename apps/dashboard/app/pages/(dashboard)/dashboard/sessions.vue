<template>
    <div>
        <Head>
            <Title>Sessions</Title>
        </Head>

        <ModalsVerifyMFA
            v-model="verifyModalOpen"
            :error="verificationError"
            :disabled
            :methods="verificationMethods"
            @got="removeAllSessions"
        />

        <div space-y-6>
            <h2>Sessions</h2>
            <div text-fs-muted-2 space-y-2>
                <p>
                    Here are all the devices that are currently logged in with your
                    {{ appConfig.site.name }} account. You can log out of each one individually or
                    all other devices.
                </p>
                <p>
                    If you see an entry you don't recognize, log out of that device and change your
                    {{ appConfig.site.name }} account password immediately.
                </p>
            </div>

            <div pt10 space-y-10>
                <div text-fs-muted-2 space-y-4>
                    <span text-sm="!" font-semibold="!">CURRENT DEVICE</span>
                    <div v-if="isLoading" flex="~ gap4">
                        <UiSkeletonLine
                            h14
                            w14
                            rounded-full="!"
                            :color-steps="['var(--fs-overlay-2)', 'var(--fs-overlay-3)']"
                        />
                        <div flex="~ col gap2" overflow-hidden>
                            <UiSkeletonLine
                                h5
                                w48
                                :color-steps="['var(--fs-overlay-2)', 'var(--fs-overlay-3)']"
                            />
                            <UiSkeletonLine
                                h4
                                w72
                                :color-steps="['var(--fs-overlay-2)', 'var(--fs-overlay-3)']"
                            />
                        </div>
                    </div>
                    <Session
                        v-else
                        :data="sessions.find((s) => s.id === currentUser!.currentSessionId)!"
                        wfit
                    />
                </div>

                <div text-fs-muted-2 space-y-4>
                    <span text-sm="!" font-semibold="!">OTHER DEVICES</span>

                    <template v-if="isLoading">
                        <div
                            v-for="i in randomNumber(1, 3)"
                            :key="i"
                            wfull
                            max-w-screen-sm
                            space-y-4
                        >
                            <div flex="~ gap4">
                                <UiSkeletonLine
                                    h14
                                    w14
                                    rounded-full="!"
                                    :color-steps="['var(--fs-overlay-2)', 'var(--fs-overlay-3)']"
                                />
                                <div flex="~ col gap2" overflow-hidden>
                                    <UiSkeletonLine
                                        h5
                                        w48
                                        :color-steps="[
                                            'var(--fs-overlay-2)',
                                            'var(--fs-overlay-3)',
                                        ]"
                                    />
                                    <UiSkeletonLine
                                        h4
                                        w72
                                        :color-steps="[
                                            'var(--fs-overlay-2)',
                                            'var(--fs-overlay-3)',
                                        ]"
                                    />
                                </div>
                            </div>
                            <UiSkeletonLine
                                h0.25
                                wfull
                                :color-steps="['var(--fs-overlay-4)', 'var(--fs-overlay-3)']"
                            />
                        </div>
                    </template>

                    <Session
                        v-for="session in sessions.filter(
                            (s) => s.id !== currentUser!.currentSessionId,
                        )"
                        v-else
                        :key="session.id"
                        :data="session"
                    />

                    <p v-if="sessions.length === 1">
                        No other devices are currently logged in with your account.
                    </p>
                </div>

                <div v-if="sessions.length > 1 && !isLoading" wfit text-fs-muted-2 space-y-4>
                    <div space-y-1>
                        <span text-sm="!" font-semibold="!">LOG OUT OF ALL KNOWN DEVICES</span>
                        <p text-sm="!">
                            You'll have to log back in on all logged out devices. Current device
                            won't be logged out.
                        </p>
                    </div>

                    <UiButton
                        variant="danger"
                        gap2
                        text-white
                        :disabled
                        :loading="disabled"
                        @click="removeAllSessions()"
                    >
                        Log out all known devices
                    </UiButton>
                </div>
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
const appConfig = useAppConfig();
const sessions = useSessions();
const currentUser = useAuthUser();
const { $toast } = useNuxtApp();

const disabled = ref(false);

const verifyModalOpen = ref(false);
const verificationMethods = ref([]);
const verificationError = ref<string>();

const isLoading = ref(!sessions.value.length);

const removeAllSessions = async (verificationData?: any) => {
    disabled.value = true;
    verificationError.value = undefined;

    try {
        await $fetch(`/api/users/@me/sessions`, {
            method: 'DELETE',
            body: { verificationData },
        });

        verifyModalOpen.value = false;

        $toast.success('All sessions removed successfully');
    } catch (error: any) {
        if (verifyModalOpen.value) {
            verificationError.value = error.data.message;
        } else {
            verifyModalOpen.value = true;
            verificationMethods.value = error.data.data.mfa.methods;
        }
    }

    disabled.value = false;
};

onMounted(async () => {
    if (!sessions.value.length) {
        const data = await $fetch(`/api/users/@me/sessions`);

        sessions.value = data.map((s) => ({
            ...s,
            lastSeen: new Date(s.lastSeen),
        }));

        isLoading.value = false;
    }
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'user-only',
});
</script>
