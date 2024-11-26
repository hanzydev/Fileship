<template>
    <ModalsEditNote v-model="editModalOpen" :data />

    <UiModal v-model="isOpen" p8 space-y-4>
        <div flex="~ justify-between" wfull>
            <h2 line-clamp-2 break-all>{{ data.title }}</h2>

            <div flex="~ gap2.5">
                <UiButton
                    v-if="currentUser?.id === data.authorId"
                    alignment="center"
                    class="h10 w10 !p0 hover:text-white"
                    icon="heroicons:pencil-16-solid"
                    icon-size="24"
                    ring="1 fs-overlay-4"
                    @click="
                        isOpen = false;
                        editModalOpen = true;
                    "
                />

                <UiButton
                    variant="accent"
                    alignment="center"
                    class="h10 w10 !p0 hover:text-white"
                    icon="heroicons-solid:x"
                    icon-size="24"
                    @click="isOpen = false"
                />
            </div>
        </div>

        <UiTextArea v-model="data!.content" label="Content" readonly wfull />

        <div flex="~ gap4 <md:col">
            <UiButton
                :icon="
                    copied ? 'heroicons-solid:clipboard-check' : 'heroicons-solid:clipboard-copy'
                "
                :icon-class="copied && 'text-green500'"
                icon-size="24"
                wfull
                gap2
                target="_blank"
                @click="handleCopy"
            >
                Copy
            </UiButton>

            <UiButton
                v-if="currentUser?.id === data.authorId"
                variant="dangerFill"
                icon="heroicons-solid:trash"
                icon-size="20"
                wfull
                gap2
                :disabled="deleting"
                :loading="deleting"
                @click="handleDelete"
            >
                Delete
            </UiButton>
        </div>
    </UiModal>
</template>

<script setup lang="ts">
import { toast } from 'vue-sonner';

const { data } = defineProps<{
    data: NoteData;
}>();

const isOpen = defineModel<boolean>({ required: true });

const currentUser = useAuthUser();
const { copied, copy } = useClipboard({ legacy: true });

const deleting = ref(false);
const editModalOpen = ref(false);

const handleDelete = async () => {
    deleting.value = true;
    await $fetch(`/api/notes/${data.id}`, { method: 'DELETE' });
    deleting.value = false;

    toast.success('Note deleted successfully');
};

const handleCopy = () => {
    copy(data.content);
    toast.success('Note copied to clipboard');
};
</script>
