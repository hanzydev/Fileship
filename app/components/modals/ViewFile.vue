<template>
    <ModalsEditFile
        v-if="currentUser?.id === data.authorId"
        v-model="editModalOpen"
        :data="data as never"
    />

    <UiModal
        v-if="isImage || isVideo || isAudio"
        v-model="isOpen"
        max-w768px
        p8
        space-y-4
    >
        <div flex="~ justify-between" wfull>
            <h2 line-clamp-2 break-all>{{ data.fileName }}</h2>

            <div flex="~ gap2.5">
                <UiButton
                    v-if="currentUser?.id === data.authorId"
                    alignment="center"
                    class="h10 w10 !p0 hover:text-white"
                    icon="heroicons:pencil-16-solid"
                    icon-size="24"
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

        <div
            grid="~ gap4"
            :class="data.views ? 'sm:grid-cols-3' : 'sm:grid-cols-2'"
        >
            <div
                v-if="data.views"
                flex="~ items-center gap2"
                rounded-md
                bg-fs3
                p4
            >
                <Icon name="heroicons-solid:eye" text-fs-accent size="24" />
                <span font-medium="!">{{ data.views.today }} today</span>
            </div>
            <div flex="~ items-center gap2" rounded-md bg-fs3 p4>
                <Icon name="mdi:sd-storage" text-fs-accent size="24" />
                <span font-medium="!">{{ data.size!.formatted }}</span>
            </div>
            <div flex="~ items-center gap2" rounded-md bg-fs3 p4>
                <Icon
                    name="heroicons-solid:calendar"
                    text-fs-accent
                    size="24"
                />
                <span font-medium="!">
                    {{ dayjs(data.createdAt).fromNow() }}
                </span>
            </div>
        </div>

        <div
            v-if="isImage"
            h96
            wfull
            cursor-pointer
            select-none
            rounded-md
            bg-contain
            bg-center
            bg-no-repeat
            :style="{
                backgroundImage: `url(/u/${data.fileName})`,
            }"
            @click="
                (event) => (event.target as HTMLDivElement).requestFullscreen()
            "
        />

        <video
            v-else-if="isVideo"
            :src="`/u/${data.fileName}`"
            controls
            h96
            wfull
            rounded-md
        />

        <audio
            v-else-if="isAudio"
            :src="`/u/${data.fileName}`"
            controls
            wfull
        />

        <div flex gap4>
            <UiButton
                icon="heroicons-solid:download"
                icon-size="24"
                wfull
                gap2
                :href="`/u/${data.fileName}?download`"
                target="_blank"
            >
                Download
            </UiButton>

            <UiButton
                :icon="
                    copied
                        ? 'heroicons-solid:clipboard-check'
                        : 'heroicons-solid:clipboard-copy'
                "
                :icon-class="copied ? 'text-green500' : 'text-slate300'"
                icon-size="20"
                wfull
                gap2
                @click="handleCopy"
            >
                Copy Link
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
import dayjs from 'dayjs';
import { toast } from 'vue-sonner';

const isOpen = defineModel<boolean>({ required: true });

const { data } = defineProps<{
    data: Partial<FileData>;
}>();

const isImage = computed(() => data.mimeType!.startsWith('image/'));
const isVideo = computed(() => data.mimeType!.startsWith('video/'));
const isAudio = computed(() => data.mimeType!.startsWith('audio/'));

const currentUser = useAuthUser();
const embed = useEmbed();

const copied = ref(false);
const deleting = ref(false);
const editModalOpen = ref(false);

let copyTimeout: NodeJS.Timeout;

const handleDelete = async () => {
    deleting.value = true;
    await $fetch(`/api/files/${data.id}`, { method: 'DELETE' });
    deleting.value = false;

    toast.success('File deleted successfully');
};

const handleCopy = () => {
    if (copyTimeout) clearTimeout(copyTimeout);

    navigator.clipboard.writeText(
        `${useRequestURL().origin}/${embed.value.enabled ? 'view' : 'u'}/${data.fileName}`,
    );

    toast.success('Link copied to clipboard');

    copied.value = true;

    copyTimeout = setTimeout(() => {
        copied.value = false;
    }, 2_000);
};
</script>
