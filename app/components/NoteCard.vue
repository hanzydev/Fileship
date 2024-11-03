<template>
    <ModalsViewNote v-model="viewModalOpen" :data />
    <ModalsEditNote v-model="editModalOpen" :data />

    <UiDropdown v-model="ctxOpen" as-ctx-menu placement="bottom">
        <div
            h100px
            wfull
            cursor-pointer
            rounded-md
            bg-fs-overlay-2
            p4
            space-y-4
            motion-safe:transition-shadow
            ring="1 fs-overlay-4"
            :class="ctxOpen ? 'cursor-default' : 'hover:(ring-1 ring-fs-accent)'"
            @click="
                ctxOpen = false;
                viewModalOpen = true;
            "
        >
            <h5 line-clamp-1 break-words text-slate400>
                {{ data.title }}
            </h5>

            <div text-slate300 space-y-2 font-medium="!">
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
                    icon="heroicons-solid:clipboard-copy"
                    icon-size="20"
                    wfull
                    gap2
                    @click="handleCopy"
                >
                    Copy
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
    data: NoteData;
}>();

const { copy } = useClipboard({ legacy: true });

const viewModalOpen = ref(false);
const editModalOpen = ref(false);

const ctxOpen = ref(false);
const deleting = ref(false);

const handleDelete = async () => {
    deleting.value = true;
    await $fetch(`/api/notes/${data.id}`, { method: 'DELETE' });
    deleting.value = false;

    toast.success('Note deleted successfully');
};

const handleCopy = () => {
    copy(data.content);
    ctxOpen.value = false;

    toast.success('Note copied to clipboard');
};
</script>
