<template>
    <ModalsFolderFiles
        v-model="filesModal.open"
        :data
        :editable="filesModal.editMode"
    />
    <ModalsEditFolder v-model="editModalOpen" :data />

    <UiDropdown v-model="ctxOpen" as-ctx-menu placement="bottom">
        <div
            h164px
            wfull
            rounded-md
            bg-fs-overlay-2
            p4
            space-y-4
            motion-safe:transition-shadow
            ring="1 fs-overlay-4"
            :class="
                ctxOpen
                    ? 'cursor-default'
                    : 'cursor-pointer hover:(ring-1 ring-fs-accent)'
            "
            @click="
                ctxOpen = false;
                filesModal.open = true;
                filesModal.editMode = false;
            "
        >
            <h5 line-clamp-1 break-words text-slate400>
                {{ data.name }}
            </h5>

            <div text-slate300 space-y-2 font-medium="!">
                <div flex="~ gap2 items-center">
                    <Icon name="heroicons-solid:document" size="20" />
                    <span>{{ data.files.length }} files</span>
                </div>

                <div flex="~ gap2 items-center">
                    <Icon
                        :name="
                            data.public
                                ? 'heroicons-solid:lock-open'
                                : 'heroicons-solid:lock-closed'
                        "
                        size="20"
                    />
                    <span>{{ data.public ? 'Public' : 'Private' }}</span>
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
            <div
                w48
                rounded-lg
                bg-fs-overlay-2
                p1.5
                space-y-1
                ring="1 fs-accent"
            >
                <UiButton
                    v-if="data.public"
                    icon="heroicons-solid:clipboard-copy"
                    icon-size="20"
                    wfull
                    gap2
                    @click="handleCopy"
                >
                    Copy Link
                </UiButton>
                <UiButton
                    icon="heroicons:pencil-16-solid"
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
                    icon="heroicons-solid:pencil-alt"
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
    data: FolderData;
}>();

const filesModal = reactive({ open: false, editMode: false });
const editModalOpen = ref(false);

const ctxOpen = ref(false);
const deleting = ref(false);

const handleDelete = async () => {
    deleting.value = true;
    await $fetch(`/api/folders/${data.id}`, { method: 'DELETE' });
    deleting.value = false;

    toast.success('Folder deleted successfully');
};

const handleCopy = () => {
    useClipboard({ legacy: true }).copy(data.publicUrl!);
    ctxOpen.value = false;

    toast.success('Link copied to clipboard');
};
</script>
