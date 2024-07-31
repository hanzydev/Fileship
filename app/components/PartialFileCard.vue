<template>
    <ModalsViewFile
        v-if="isImage || isVideo || isAudio"
        v-model="viewModalOpen"
        :data
    />

    <UiDropdown
        v-model="ctxOpen"
        as-ctx-menu
        placement="bottom"
        trigger-class="hfull"
    >
        <div
            v-if="isImage || isVideo"
            wfull
            rounded-md
            bg-fs-overlay-2
            motion-safe:transition-shadow
            :class="
                ctxOpen
                    ? 'cursor-default'
                    : 'cursor-pointer hover:(ring-1 ring-fs-accent)'
            "
            @click="viewModalOpen = true"
        >
            <div
                v-if="isImage || isVideo"
                relative
                h52
                overflow-hidden
                rounded-md
                p4
                text-center
                flex="~ col items-center justify-center gap2"
            >
                <img
                    v-if="isImage"
                    :src="`/u/${data.fileName}`"
                    :alt="data.fileName"
                    absolute
                    hfull
                    wfull
                    object-contain
                    hover:scale-105
                    motion-safe:transition-transform
                />
                <video
                    v-if="isVideo"
                    :src="`/u/${data.fileName}`"
                    :alt="data.fileName"
                    absolute
                    hfull
                    wfull
                    object-contain
                    hover:scale-105
                    motion-safe:transition-transform
                />
            </div>
        </div>
        <div
            v-else
            h52
            wfull
            rounded-md
            bg-fs-overlay-2
            p8
            space-y-8
            motion-safe:transition-shadow
            :class="{
                'hover:(ring-1 ring-fs-accent)': isAudio && !ctxOpen,
                'cursor-default': ctxOpen,
                'cursor-pointer': isAudio && !ctxOpen,
            }"
            @click="isAudio && (viewModalOpen = true)"
        >
            <h5 line-clamp-1 break-words text-slate400>
                {{ data.fileName }}
            </h5>

            <div
                flex="~ col justify-between gap2"
                justify-between
                text-slate300
                font-medium="!"
            >
                <div flex="~ gap2 items-center">
                    <Icon name="mdi:sd-storage" size="20" />
                    <span>{{ data.size!.formatted }}</span>
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
                    icon="heroicons:eye-16-solid"
                    icon-size="20"
                    wfull
                    gap2
                    :href="`/view/${data.fileName}`"
                >
                    Open
                </UiButton>
                <UiButton
                    icon="heroicons-solid:clipboard-copy"
                    icon-size="20"
                    wfull
                    gap2
                    @click="handleCopy"
                >
                    Copy Link
                </UiButton>
                <UiButton
                    icon="heroicons-solid:download"
                    icon-size="20"
                    wfull
                    gap2
                    :href="`/u/${data.fileName}?download`"
                    target="_blank"
                >
                    Download
                </UiButton>
            </div>
        </template>
    </UiDropdown>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';
import { toast } from 'vue-sonner';

const { data } = defineProps<{
    data: Partial<FileData>;
}>();

const isImage = computed(() => data.mimeType!.startsWith('image/'));
const isVideo = computed(() => data.mimeType!.startsWith('video/'));
const isAudio = computed(() => data.mimeType!.startsWith('audio/'));

const viewModalOpen = ref(false);
const ctxOpen = ref(false);

const handleCopy = () => {
    navigator.clipboard.writeText(
        `${useRequestURL().origin}/u/${data.fileName}`,
    );
    ctxOpen.value = false;

    toast.success('Link copied to clipboard');
};
</script>
