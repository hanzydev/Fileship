<template>
    <div>
        <ModalsVerifyMFA
            v-model="verifyModalOpen"
            :error="verificationError"
            :disabled
            :methods="verificationMethods"
            @got="handleRemoveSession"
        />

        <div wfull max-w-screen-sm space-y-4>
            <div flex="~ gap4">
                <div relative>
                    <div flex="~ items-center justify-center" h14 w14 rounded-full bg-fs-overlay-2>
                        <Icon
                            :name="
                                data.os === 'Mobile'
                                    ? 'fluent:phone-24-filled'
                                    : 'fluent:desktop-24-filled'
                            "
                            size="24"
                            text-fs-muted-2
                        />
                    </div>
                    <div
                        v-if="data.platform !== 'Unknown'"
                        flex="~ items-center justify-center"
                        absolute
                        bottom--0.5
                        right--0.5
                        h6
                        w6
                        rounded-full
                        bg-fs-overlay-1
                    >
                        <Icon
                            :name="
                                data.platform === 'Chrome'
                                    ? 'mingcute:chrome-fill'
                                    : data.platform === 'Firefox'
                                      ? 'mingcute:firefox-fill'
                                      : data.platform === 'OPR'
                                        ? 'mdi:opera'
                                        : data.platform === 'Edge'
                                          ? 'mingcute:edge-fill'
                                          : 'mingcute:safari-fill'
                            "
                            size="18"
                        />
                    </div>
                </div>
                <div flex="~ col gap2" overflow-hidden>
                    <span text-sm="!" font-semibold="!" break-words uppercase>
                        {{ data.os }} - {{ data.platform }} ·
                        {{ data.ip }}
                    </span>
                    <span text-sm="!" font-medium="!">
                        {{ data.location }} ·
                        {{ dayjs(data.lastSeen).fromNow() }}
                    </span>
                </div>
                <button
                    v-if="data.id !== currentUser!.currentSessionId"
                    mla
                    text-fs-muted-1
                    aria-label="Remove session"
                    :disabled
                    :class="disabled && 'cursor-not-allowed op50'"
                    @click="handleRemoveSession()"
                >
                    <Icon name="heroicons-solid:x" size="24" />
                </button>
            </div>
            <UiDivider v-if="data.id !== currentUser!.currentSessionId" />
        </div>
    </div>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

const currentUser = useAuthUser();
const { $toast } = useNuxtApp();

const { data } = defineProps<{
    data: SessionData;
}>();

const disabled = ref(false);

const verifyModalOpen = ref(false);
const verificationError = ref<string>();
const verificationMethods = ref([]);

const handleRemoveSession = async (verificationData?: any) => {
    disabled.value = true;
    verificationError.value = undefined;

    try {
        await $fetch(`/api/users/@me/sessions/${data.id}`, {
            method: 'DELETE',
            body: { verificationData },
        });

        verifyModalOpen.value = false;

        $toast.success('Session removed successfully');
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
</script>
