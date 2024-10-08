<template>
    <ModalsViewFile v-if="canBeViewed" v-model="viewModalOpen" :data />

    <ModalsEditFile
        v-if="currentUser?.id === data.authorId"
        v-model="editModalOpen"
        :data
    />

    <UiDropdown
        v-model="ctxOpen"
        as-ctx-menu
        placement="bottom"
        trigger-class="hfull"
    >
        <div
            relative
            rounded-md
            motion-safe:transition-shadow
            :class="[
                selected && '!ring-2 ring-fs-accent',
                !selectable && (ctxOpen || !canBeViewed)
                    ? 'cursor-default'
                    : 'cursor-pointer hover:(ring-1 ring-fs-accent)',
            ]"
            @click="
                selectable
                    ? (selected = !selected)
                    : canBeViewed && (viewModalOpen = true)
            "
        >
            <Transition
                enter-active-class="motion-safe:(animate-in fade-in)"
                leave-active-class="motion-safe:(animate-out fade-out)"
            >
                <div
                    v-if="selected"
                    flex="~ items-center justify-center"
                    absolute
                    z10
                    hfull
                    wfull
                    cursor-pointer
                    rounded-md
                    backdrop-blur-sm
                    bg="[color-mix(in_srgb,_var(--fs-accent),_transparent_90%)]"
                >
                    <Icon name="heroicons-solid:check" size="56" />
                </div>
            </Transition>
            <Icon
                v-if="isVideo && !selected"
                name="heroicons:play-solid"
                size="64"
                absolute
                z10
                top="1/2"
                left="1/2"
                translate-y-="1/2"
                translate-x-="1/2"
                op75
            />
            <div
                v-if="isImage || isVideo"
                relative
                h208px
                wfull
                flex="~ col items-center justify-center gap2"
                overflow-hidden
                rounded-md
                bg-fs-overlay-2
                p4
                text-center
            >
                <img
                    v-if="isImage"
                    :src="data.directUrl"
                    :alt="data.fileName"
                    :class="!selectable && 'hover:scale-105'"
                    absolute
                    hfull
                    wfull
                    object-contain
                    motion-safe:transition-transform
                />
                <video
                    v-if="isVideo"
                    :src="data.directUrl"
                    :alt="data.fileName"
                    :class="!selectable && 'hover:scale-105'"
                    absolute
                    hfull
                    wfull
                    object-contain
                    motion-safe:transition-transform
                />
            </div>
            <div v-else h208px wfull rounded-md bg-fs-overlay-2 p8 space-y-8>
                <h5 line-clamp-1 break-words text-slate400>
                    {{ data.fileName }}
                </h5>

                <div
                    flex="~ col justify-between gap2"
                    text-slate-300
                    font-medium
                >
                    <div flex="~ gap2 items-center">
                        <Icon name="heroicons-solid:eye" size="20" />
                        <span>{{ data.views.today }} today</span>
                    </div>

                    <div flex="~ gap2 items-center">
                        <Icon name="mdi:sd-storage" size="20" />
                        <span>{{ data.size.formatted }}</span>
                    </div>

                    <div flex="~ gap2 items-center">
                        <Icon name="heroicons-solid:calendar" size="20" />
                        <span>
                            {{
                                dayjs(data.createdAt).format(
                                    'MMM D, YYYY h:mm A',
                                )
                            }}
                        </span>
                    </div>
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
                    v-if="!selectable || canBeViewed"
                    icon="heroicons:eye-16-solid"
                    icon-size="20"
                    wfull
                    gap2
                    :href="selectable ? undefined : data.embedUrl"
                    @click="
                        if (selectable) {
                            ctxOpen = false;
                            viewModalOpen = true;
                        }
                    "
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
                    :href="`${data.directUrl}?download`"
                    target="_blank"
                >
                    Download
                </UiButton>
                <UiButton
                    v-if="currentUser?.id === data.authorId"
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
                    v-if="
                        currentUser?.id === data.authorId &&
                        data.folderId &&
                        !selectable
                    "
                    icon="heroicons-solid:folder-remove"
                    icon-size="20"
                    wfull
                    gap2
                    :disabled="updating"
                    @click="handleMoveFile(null)"
                >
                    Take Out
                </UiButton>
                <UiDropdown
                    v-if="
                        currentUser?.id === data.authorId &&
                        !selectable &&
                        !data.folderId
                    "
                    placement="right"
                    hover
                >
                    <UiButton
                        icon="heroicons-solid:folder-add"
                        icon-size="20"
                        wfull
                        gap2
                        :disabled="updating"
                    >
                        Add to Folder
                    </UiButton>
                    <template #content>
                        <div
                            h64
                            w64
                            overflow-y-auto
                            rounded-lg
                            bg-fs-overlay-2
                            p1.5
                            ring="1 fs-accent"
                            space-y-2
                        >
                            <UiSearchBar
                                v-model="addToFolderSearchQuery"
                                placeholder="Search folders..."
                                input-class="!h10 !bg-fs-overlay-3 !ring-0"
                            />

                            <p
                                v-if="
                                    !results.length && !addToFolderSearchQuery
                                "
                                mx4
                                translate-y-16
                                text-center
                                text-slate300
                            >
                                There are no folders to display.
                            </p>

                            <div space-y-1>
                                <UiButton
                                    v-if="
                                        addToFolderSearchQuery &&
                                        !results.find(
                                            (r) =>
                                                r.item.name ===
                                                addToFolderSearchQuery.trim(),
                                        )
                                    "
                                    :disabled="updating"
                                    wfull
                                    break-all
                                    @click="handleCreateFolderWithFile"
                                >
                                    Create folder "{{ addToFolderSearchQuery }}"
                                </UiButton>

                                <UiButton
                                    v-for="(folder, index) in results.map(
                                        (r) => r.item,
                                    )"
                                    :key="index"
                                    :disabled="updating"
                                    wfull
                                    gap2
                                    break-all
                                    @click="handleMoveFile(folder.id)"
                                >
                                    {{ folder.name }}
                                </UiButton>
                            </div>
                        </div>
                    </template>
                </UiDropdown>
                <UiButton
                    v-if="currentUser?.id === data.authorId"
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

import { useFuse } from '@vueuse/integrations/useFuse';

const { data } = defineProps<{
    data: FileData;
    selectable?: boolean;
}>();

const selected = defineModel<boolean>('selected', {
    required: false,
    default: false,
});

const currentUser = useAuthUser();
const folders = useFolders();
const embed = useEmbed();

const addToFolderSearchQuery = ref('');

const { results } = useFuse(addToFolderSearchQuery, folders, {
    matchAllWhenSearchEmpty: true,
    fuseOptions: {
        keys: ['name'],
    },
});

const isImage = computed(() => data.mimeType.startsWith('image/'));
const isVideo = computed(() => data.mimeType.startsWith('video/'));
const isAudio = computed(() => data.mimeType.startsWith('audio/'));

const canBeViewed = computed(
    () => isImage.value || isVideo.value || isAudio.value,
);

const viewModalOpen = ref(false);
const editModalOpen = ref(false);

const ctxOpen = ref(false);
const deleting = ref(false);
const updating = ref(false);

const handleDelete = async () => {
    deleting.value = true;
    await $fetch(`/api/files/${data.id}`, { method: 'DELETE' });
    deleting.value = false;

    toast.success('File deleted successfully');
};

const handleCopy = () => {
    useClipboard({ legacy: true }).copy(
        embed.value.enabled ? data.embedUrl : data.directUrl,
    );
    ctxOpen.value = false;

    toast.success('Link copied to clipboard');
};

const handleMoveFile = async (folderId: string | null) => {
    updating.value = true;

    await $fetch(`/api/files/${data.id}`, {
        method: 'PATCH',
        body: {
            folderId,
        },
    });

    updating.value = false;

    toast.success('File moved successfully');
};

const handleCreateFolderWithFile = async () => {
    updating.value = true;

    try {
        await $fetch('/api/folders', {
            method: 'POST',
            body: {
                name: addToFolderSearchQuery.value.trim(),
                files: [data.id],
            },
        });

        toast.success('Folder created and file added successfully');
    } catch (error: any) {
        if (error.data.data) toast.error(error.data.data.name._errors[0]);
        else toast.error(error.data.message);
    }

    updating.value = false;
};
</script>
