<template>
    <ModalsViewNote v-model="viewModalOpen" :data />
    <ModalsEditNote v-model="editModalOpen" :data />

    <UiDropdown v-model="ctxOpen" as-ctx-menu placement="bottom">
        <div
            h100px
            wfull
            cursor-pointer
            rounded-xl
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
            <h5 line-clamp-1 break-words text-fs-muted-3>
                {{ data.title }}
            </h5>

            <div text-fs-muted-2 space-y-2 font-medium="!">
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
                    variant="onOverlay"
                    icon="solar:clipboard-bold"
                    icon-size="20"
                    wfull
                    gap2
                    @click="handleCopy"
                >
                    Copy
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
    data: NoteData;
}>();

const { copy } = useClipboard({ legacy: true });
const { $toast } = useNuxtApp();

const viewModalOpen = ref(false);
const editModalOpen = ref(false);

const ctxOpen = ref(false);
const deleting = ref(false);

const handleDelete = async () => {
    deleting.value = true;

    try {
        await $fetch(`/api/notes/${data.id}`, { method: 'DELETE' });
        $toast.success('Note deleted successfully');
    } catch (error: any) {
        $toast.error(error.data.message);
    }

    deleting.value = false;
};

const handleCopy = () => {
    copy(data.content);
    ctxOpen.value = false;

    $toast.success('Note copied to clipboard');
};
</script>
