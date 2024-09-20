<template>
    <UiModal v-model="noteModalOpen" p8 space-y-4>
        <div flex="~ justify-between" wfull>
            <h2 line-clamp-2 break-all>{{ data.title }}</h2>

            <div flex="~ gap2.5">
                <UiButton
                    v-if="currentUser?.id === data.authorId"
                    alignment="center"
                    class="h10 w10 !p0 hover:text-white"
                    icon="heroicons:pencil-16-solid"
                    icon-size="24"
                    @click="
                        noteModalOpen = false;
                        editModalOpen = true;
                    "
                />

                <UiButton
                    variant="accent"
                    alignment="center"
                    class="h10 w10 !p0 hover:text-white"
                    icon="heroicons-solid:x"
                    icon-size="24"
                    @click="noteModalOpen = false"
                />
            </div>
        </div>

        <UiTextArea v-model="data!.content" label="Content" readonly wfull />

        <div flex="~ gap4 max-md:col">
            <UiButton
                :icon="
                    copied
                        ? 'heroicons-solid:clipboard-check'
                        : 'heroicons-solid:clipboard-copy'
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
            :class="
                ctxOpen ? 'cursor-default' : 'hover:(ring-1 ring-fs-accent)'
            "
            @click="
                ctxOpen = false;
                noteModalOpen = true;
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
            <div
                w48
                rounded-lg
                bg-fs-overlay-2
                p1.5
                space-y-1
                ring="1 fs-accent"
            >
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

const currentUser = useAuthUser();
const { copied, copy } = useClipboard({ legacy: true });

const noteModalOpen = ref(false);
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
