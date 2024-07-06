<template>
    <div>
        <Head>
            <Title>Sessions</Title>
        </Head>

        <ModalsVerifyTotp
            v-if="currentUser!.totpEnabled"
            v-model="verifyModalOpen"
            :error
            :disabled
            @got="removeAllSessions"
        />
        <ModalsVerifyUserPassword
            v-else
            v-model="verifyModalOpen"
            :error
            :disabled
            @got="removeAllSessions"
        />

        <div space-y-6>
            <h2>Sessions</h2>
            <div text-slate300 space-y-2>
                <p>
                    Here are all the devices that are currently logged in with
                    your
                    {{ appConfig.site.name }} account. You can log out of each
                    one individually or all other devices.
                </p>
                <p>
                    If you see an entry you don't recognize, log out of that
                    device and change your {{ appConfig.site.name }} account
                    password immediately.
                </p>
            </div>

            <div pt10 space-y-10>
                <div text-slate300 space-y-4>
                    <span text-sm="!" font-semibold="!">CURRENT DEVICE</span>
                    <Session
                        :data="
                            sessions.find(
                                (s) => s.id === currentUser!.currentSessionId,
                            )!
                        "
                        wfit
                    />
                </div>

                <div text-slate300 space-y-4>
                    <span text-sm="!" font-semibold="!">OTHER DEVICES</span>

                    <Session
                        v-for="session in sessions.filter(
                            (s) => s.id !== currentUser!.currentSessionId,
                        )"
                        :key="session.id"
                        :data="session"
                    />

                    <p v-if="sessions.length === 1">
                        No other devices are currently logged in with your
                        account.
                    </p>
                </div>

                <div v-if="sessions.length > 1" wfit text-slate300 space-y-4>
                    <div space-y-1>
                        <span text-sm="!" font-semibold="!">
                            LOG OUT OF ALL KNOWN DEVICES
                        </span>
                        <p text-sm="!">
                            You'll have to log back in on all logged out
                            devices. Current device won't be logged out.
                        </p>
                    </div>

                    <UiButton
                        variant="danger"
                        :disabled
                        text-white
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
import { toast } from 'vue-sonner';

const appConfig = useAppConfig();
const sessions = useSessions();
const currentUser = useAuthUser();

const { data } = await useFetch(`/api/users/sessions`);

sessions.value = data.value!.map((s) => ({
    ...s,
    lastSeen: new Date(s.lastSeen),
}));

const error = ref<string>();
const disabled = ref(false);
const verifyModalOpen = ref(false);

const removeAllSessions = async (verificationData?: string) => {
    disabled.value = true;
    error.value = undefined;

    try {
        await $fetch(`/api/users/sessions`, {
            method: 'DELETE',
            body: { verificationData },
        });

        verifyModalOpen.value = false;

        toast.success('All sessions removed successfully');
    } catch (_error: any) {
        if (verifyModalOpen.value) error.value = _error.data.message;
        else verifyModalOpen.value = true;
    }

    disabled.value = false;
};

definePageMeta({
    layout: 'dashboard',
    middleware: 'user-only',
});
</script>
