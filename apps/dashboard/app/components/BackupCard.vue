<template>
    <ModalsAreYouSure
        v-model="areYouSureModalOpen"
        title="Are you really sure you want to load this backup?"
        description="Your current data will be lost and replaced with the backup data."
        @confirm="handleLoad"
    />

    <ModalsVerifyMFA
        v-model="verifyModalOpen"
        :error="verificationError"
        :disabled="loading"
        :methods="verificationMethods"
        @got="handleLoad"
    />

    <UiDropdown v-model="ctxOpen" as-ctx-menu placement="bottom">
        <div
            h132px
            wfull
            rounded-xl
            bg-fs-overlay-2
            p4
            space-y-4
            motion-safe:transition-shadow
            ring="1 fs-overlay-4"
            :class="ctxOpen ? 'cursor-default' : 'cursor-pointer hover:(ring-1 ring-fs-accent)'"
            @click="
                ctxOpen = false;
                areYouSureModalOpen = true;
            "
        >
            <h5 line-clamp-1 break-words text-fs-muted-3>
                {{ data.id }}
            </h5>

            <div text-fs-muted-2 font-medium space-y-2>
                <div flex="~ gap2 items-center">
                    <Icon name="solar:diskette-bold" size="20" />
                    <span>
                        {{ data.size.formatted }}
                    </span>
                </div>

                <div flex="~ gap2 items-center">
                    <Icon name="solar:calendar-bold" size="20" />
                    <span>
                        {{ dayjs(data.createdAt).format('MMM D, YYYY h:mm A') }}
                    </span>
                </div>
            </div>
        </div>
        <template #content>
            <div w48 rounded-xl bg-fs-overlay-2 p1.5 space-y-1 ring="1 fs-overlay-4">
                <UiButton
                    variant="onOverlay"
                    icon="solar:download-minimalistic-bold"
                    icon-size="20"
                    wfull
                    gap2
                    :href="`/api/users/@me/backups/${data.id}`"
                    target="_blank"
                >
                    Download
                </UiButton>
                <UiButton
                    variant="onOverlay"
                    icon="solar:trash-bin-minimalistic-bold"
                    icon-size="20"
                    wfull
                    gap2
                    text-red-500
                    :disabled="deleting"
                    @click="handleDelete"
                >
                    Delete
                </UiButton>
            </div>
        </template>
    </UiDropdown>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

const { data } = defineProps<{
    data: BackupData;
}>();

const { $toast } = useNuxtApp();

const areYouSureModalOpen = ref(false);
const ctxOpen = ref(false);
const deleting = ref(false);
const loading = ref(false);

const verifyModalOpen = ref(false);
const verificationError = ref<string>();
const verificationMethods = ref([]);

const handleDelete = async () => {
    deleting.value = true;

    try {
        await $fetch(`/api/users/@me/backups/${data.id}`, { method: 'DELETE' });
        $toast.success('Backup deleted successfully');
    } catch (error: any) {
        $toast.error(error.data.message);
    }

    deleting.value = false;
};

const handleLoad = async (verificationData?: any) => {
    loading.value = true;
    verificationError.value = undefined;

    try {
        await $fetch(`/api/users/@me/backups/${data.id}/load`, {
            method: 'POST',
            body: { verificationData },
        });

        verifyModalOpen.value = false;

        $toast.success('Backup is being loaded, this may take a while');
    } catch (error: any) {
        if (verifyModalOpen.value) {
            verificationError.value = error.data.message;
        } else {
            verifyModalOpen.value = true;
            verificationMethods.value = error.data.data.mfa.methods;
        }
    }

    loading.value = false;
};
</script>
