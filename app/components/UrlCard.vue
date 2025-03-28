<template>
    <ModalsEditUrl v-model="editModalOpen" :data />

    <UiDropdown v-model="ctxOpen" as-ctx-menu placement="bottom">
        <a
            block
            h132px
            wfull
            rounded-md
            bg-fs-overlay-2
            p4
            space-y-4
            motion-safe:transition-shadow
            ring="1 fs-overlay-4"
            :class="ctxOpen ? 'cursor-default' : 'hover:(ring-1 ring-fs-accent)'"
            :href="data.destinationUrl"
            target="_blank"
        >
            <h5 line-clamp-1 break-words text-slate400>
                {{ data.vanity }}
            </h5>

            <div text-slate300 space-y-2 font-medium="!">
                <div flex="~ gap2 items-center">
                    <Icon name="heroicons-solid:eye" size="20" />
                    <span>{{ data.views.today }} today</span>
                </div>

                <div flex="~ gap2 items-center">
                    <Icon name="heroicons-solid:calendar" size="20" />
                    <span>
                        {{ dayjs(data.createdAt).format('MMM D, YYYY h:mm A') }}
                    </span>
                </div>
            </div>
        </a>
        <template #content>
            <div w48 rounded-lg bg-fs-overlay-2 p1.5 space-y-1 ring="1 fs-overlay-4">
                <UiButton
                    variant="onOverlay"
                    icon="heroicons-solid:clipboard-copy"
                    icon-size="20"
                    wfull
                    gap2
                    @click="handleCopy"
                >
                    Copy Link
                </UiButton>
                <UiButton
                    variant="onOverlay"
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
    data: UrlData;
}>();

const ctxOpen = ref(false);
const editModalOpen = ref(false);

const deleting = ref(false);

const handleDelete = async () => {
    deleting.value = true;
    await $fetch(`/api/urls/${data.id}`, { method: 'DELETE' });
    deleting.value = false;

    toast.success('URL deleted successfully');
};

const handleCopy = () => {
    useClipboard({ legacy: true }).copy(data.url);
    ctxOpen.value = false;

    toast.success('Link copied to clipboard');
};
</script>
