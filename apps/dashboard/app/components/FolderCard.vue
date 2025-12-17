<template>
    <ModalsFolderFiles v-model="filesModal.open" v-model:edit-mode="filesModal.editMode" :data />
    <ModalsEditFolder v-model="editModalOpen" :data />

    <ModalsAreYouSure
        v-model="areYouSureModalOpen"
        title="Delete Folder"
        description="Are you sure you want to delete this folder?"
        @confirm="handleDelete"
    >
        <template #extra>
            <div v-if="data.files.length" flex="~ gap2 items-center">
                <UiSwitch v-model="deleteFilesToo" :disabled="deleting" />
                <span text-fs-muted-1 font-medium="!">Delete files too</span>
            </div>
        </template>
    </ModalsAreYouSure>

    <UiDropdown v-model="ctxOpen" as-ctx-menu placement="bottom">
        <div
            h164px
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
                filesModal.open = true;
                filesModal.editMode = false;
            "
        >
            <h5 line-clamp-1 break-words text-fs-muted-3>
                {{ data.name }}
            </h5>

            <div text-fs-muted-2 space-y-2 font-medium="!">
                <div flex="~ gap2 items-center">
                    <Icon name="solar:documents-bold" size="20" />
                    <span>{{ data.files.length }} files</span>
                </div>

                <div flex="~ gap2 items-center">
                    <Icon
                        :name="
                            data.public
                                ? 'solar:lock-keyhole-minimalistic-unlocked-bold'
                                : 'solar:lock-keyhole-minimalistic-bold'
                        "
                        size="20"
                    />
                    <span>{{ data.public ? 'Public' : 'Private' }}</span>
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
            <div w48 rounded-xl bg-fs-overlay-2 p1 space-y-1 ring="1 fs-overlay-4">
                <UiButton
                    v-if="data.public"
                    variant="onOverlay"
                    icon="solar:clipboard-bold"
                    icon-size="20"
                    wfull
                    gap2
                    @click="handleCopy"
                >
                    Copy Link
                </UiButton>
                <UiButton
                    variant="onOverlay"
                    icon="solar:pen-2-bold"
                    icon-size="20"
                    wfull
                    gap2
                    @click="
                        ctxOpen = false;
                        editModalOpen = true;
                    "
                >
                    Edit
                </UiButton>
                <UiButton
                    variant="onOverlay"
                    icon="solar:gallery-edit-bold"
                    icon-size="20"
                    wfull
                    gap2
                    @click="
                        ctxOpen = false;
                        filesModal.open = true;
                        filesModal.editMode = true;
                    "
                >
                    Edit Files
                </UiButton>
                <UiButton
                    variant="onOverlay"
                    icon="solar:trash-bin-minimalistic-bold"
                    icon-size="20"
                    wfull
                    gap2
                    text-red-500
                    :disabled="deleting"
                    @click="
                        ctxOpen = false;
                        areYouSureModalOpen = true;
                    "
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
    data: FolderData;
}>();

const { $toast } = useNuxtApp();

const filesModal = reactive({ open: false, editMode: false });
const editModalOpen = ref(false);
const ctxOpen = ref(false);
const deleting = ref(false);
const areYouSureModalOpen = ref(false);
const deleteFilesToo = ref(true);

const handleDelete = async () => {
    deleting.value = true;

    try {
        await $fetch(`/api/folders/${data.id}`, {
            method: 'DELETE',
            body: { deleteFilesToo: deleteFilesToo.value },
        });

        $toast.success(
            deleteFilesToo.value
                ? 'Folder and its files deleted successfully'
                : 'Folder deleted successfully',
        );
    } catch (error: any) {
        $toast.error(error.data.message);
    }

    deleting.value = false;
};

const handleCopy = () => {
    useClipboard({ legacy: true }).copy(data.publicUrl!);
    ctxOpen.value = false;

    $toast.success('Link copied to clipboard');
};
</script>
