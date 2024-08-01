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
                enter-active-class="motion-safe:animate-in motion-safe:fade-in"
                leave-active-class="motion-safe:animate-out motion-safe:fade-out"
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
                opacity-75
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
                    :src="`/u/${data.fileName}`"
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
                    :src="`/u/${data.fileName}`"
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
                    :href="selectable ? undefined : `/view/${data.fileName}`"
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
                    :href="`/u/${data.fileName}?download`"
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
                    @click="changeFolder(null)"
                >
                    Take Out
                </UiButton>
                <UiDropdown
                    v-if="
                        currentUser?.id === data.authorId &&
                        !selectable &&
                        !data.folderId
                    "
                    :placement="width < 768 ? 'bottom' : 'right'"
                    hover
                    md:pl4
                    lt-md:-left-10
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
                                    !filteredFolders.length &&
                                    !addToFolderSearchQuery
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
                                    v-if="addToFolderSearchQuery"
                                    :disabled="updating"
                                    wfull
                                    break-all
                                    @click="createFolderAndAdd"
                                >
                                    Create folder "{{ addToFolderSearchQuery }}"
                                </UiButton>

                                <UiButton
                                    v-for="(folder, index) in filteredFolders"
                                    :key="index"
                                    :disabled="updating"
                                    wfull
                                    gap2
                                    @click="changeFolder(folder.id)"
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

const props = defineProps<{
    data: FileData;
    selectable?: boolean;
}>();

const { data } = toRefs(props);

const selected = defineModel<boolean>('selected', {
    required: false,
    default: false,
});

const { width } = useWindowSize();

const currentUser = useAuthUser();
const folders = useFolders();
const embed = useEmbed();

const addToFolderSearchQuery = ref('');

const filteredFolders = computed(() =>
    folders.value.filter((l) =>
        Object.values(l).some((v) =>
            v
                ?.toString()
                ?.toLowerCase()
                ?.includes(addToFolderSearchQuery.value.toLowerCase()),
        ),
    ),
);

const isImage = computed(() => data.value.mimeType.startsWith('image/'));
const isVideo = computed(() => data.value.mimeType.startsWith('video/'));
const isAudio = computed(() => data.value.mimeType.startsWith('audio/'));

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
    await $fetch(`/api/files/${data.value.id}`, { method: 'DELETE' });
    deleting.value = false;

    toast.success('File deleted successfully');
};

const handleCopy = () => {
    navigator.clipboard.writeText(
        `${useRequestURL().origin}/${embed.value.enabled ? 'view' : 'u'}/${data.value.fileName}`,
    );
    ctxOpen.value = false;

    toast.success('Link copied to clipboard');
};

const changeFolder = async (folderId: string | null) => {
    updating.value = true;

    await $fetch(`/api/files/${data.value.id}`, {
        method: 'PATCH',
        body: {
            folderId,
        },
    });

    updating.value = false;

    toast.success('File moved successfully');
};

const createFolderAndAdd = async () => {
    updating.value = true;

    try {
        await $fetch('/api/folders', {
            method: 'POST',
            body: {
                name: addToFolderSearchQuery.value,
                public: false,
                files: [data.value.id],
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
