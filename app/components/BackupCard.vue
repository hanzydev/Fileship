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
            rounded-md
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
            <h5 line-clamp-1 break-words text-slate400>
                {{ data.id }}
            </h5>

            <div text-slate300 font-medium space-y-2>
                <div flex="~ gap2 items-center">
                    <Icon name="mdi:sd-storage" size="20" />
                    <span>
                        {{ data.size.formatted }}
                    </span>
                </div>

                <div flex="~ gap2 items-center">
                    <Icon name="heroicons-solid:calendar" size="20" />
                    <span>
                        {{ dayjs(data.createdAt).format('MMM D, YYYY h:mm A') }}
                    </span>
                </div>
            </div>
        </div>
        <template #content>
            <div w48 rounded-lg bg-fs-overlay-2 p1.5 space-y-1 ring="1 fs-accent">
                <UiButton
                    variant="onOverlay"
                    icon="heroicons-solid:download"
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
                    icon="heroicons-solid:trash"
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
import { toast } from 'vue-sonner';

const { data } = defineProps<{
    data: BackupData;
}>();

const areYouSureModalOpen = ref(false);
const ctxOpen = ref(false);
const deleting = ref(false);
const loading = ref(false);

const verifyModalOpen = ref(false);
const verificationError = ref<string>();
const verificationMethods = ref([]);

const handleDelete = async () => {
    deleting.value = true;
    await $fetch(`/api/users/@me/backups/${data.id}`, { method: 'DELETE' });
    deleting.value = false;

    toast.success('Backup deleted successfully');
};

const handleLoad = async (verificationData?: any) => {
    loading.value = true;
    verificationError.value = undefined;

    try {
        await $fetch(`/api/users/@me/backups/${data.id}`, {
            method: 'PUT',
            body: { verificationData },
        });

        verifyModalOpen.value = false;

        toast.success('Backup is being loaded, this may take a while');
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
