<template>
    <ModalsEditNote v-model="editModalOpen" :data />

    <Teleport to="body">
        <Transition
            enter-active-class="motion-safe:(animate-in fade-in zoom-in-95 slide-in-top-48%)"
            leave-active-class="motion-safe:(animate-out fade-out zoom-out-95 slide-out-top-48%)"
        >
            <UiButton
                v-if="isOpen && notes.length > 1"
                variant="secondary"
                alignment="center"
                icon="solar:arrow-left-linear"
                icon-size="24"
                rounded-2xl="!"
                p0="!"
                aria-label="Previous"
                top="1/2"
                translate-y-="1/2"
                data-ignore-modal-outer-click
                absolute
                left-8
                z60
                size-11
                :disabled="isFirst || !prev"
                @click="handlePrev"
            />
        </Transition>
        <Transition
            enter-active-class="motion-safe:(animate-in fade-in zoom-in-95 slide-in-top-48%)"
            leave-active-class="motion-safe:(animate-out fade-out zoom-out-95 slide-out-top-48%)"
        >
            <UiButton
                v-if="isOpen && notes.length > 1"
                variant="secondary"
                alignment="center"
                icon="solar:arrow-right-linear"
                icon-size="24"
                rounded-2xl="!"
                p0="!"
                aria-label="Previous"
                top="1/2"
                translate-y-="1/2"
                data-ignore-modal-outer-click
                absolute
                right-8
                z60
                size-11
                :disabled="isLast || !next"
                @click="handleNext"
            />
        </Transition>
    </Teleport>

    <UiModal :id="modalId" v-model="isOpen" p8 space-y-4>
        <div flex="~ justify-between" wfull>
            <h2 line-clamp-2 break-all>{{ data.title }}</h2>

            <div flex="~ gap2.5">
                <UiButton
                    v-if="currentUser?.id === data.authorId"
                    alignment="center"
                    class="h10 w10 !p0 hover:text-white"
                    icon="solar:pen-2-bold"
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
                    icon="lucide:x"
                    icon-size="24"
                    @click="isOpen = false"
                />
            </div>
        </div>

        <UiTextArea v-model="data!.content" label="Content" readonly wfull cursor-text="!" />

        <div flex="~ gap4 <md:col">
            <UiButton
                :icon="copied ? 'solar:clipboard-check-bold' : 'solar:clipboard-bold'"
                :icon-class="copied && 'text-green500!'"
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
                icon="solar:trash-bin-minimalistic-bold"
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
import { Cubic, gsap } from 'gsap';

const { data: _data } = defineProps<{
    data: NoteData;
}>();

const isOpen = defineModel<boolean>({ required: true });

const data = ref(_data);

const notes = useNotes();
const currentUser = useAuthUser();
const { copied, copy } = useClipboard({ legacy: true });
const reducedMotion = usePreferredReducedMotion();
const { $toast } = useNuxtApp();
const modalId = useId();

const index = computed(() => notes.value.findIndex((note) => note.id === data.value.id));
const next = computed(() => notes.value[index.value + 1]);
const prev = computed(() => notes.value[index.value - 1]);
const isFirst = computed(() => index.value === 0);
const isLast = computed(() => index.value === notes.value.length - 1);
const tl = computed(() =>
    reducedMotion.value === 'no-preference' && isOpen.value
        ? gsap.timeline({ paused: true, reversed: true }).to(`#${modalId}`, {
              opacity: 0,
              duration: 0.15,
              scale: 0.95,
              y: '2%',
              ease: Cubic.easeIn,
          })
        : null,
);

const deleting = ref(false);
const editModalOpen = ref(false);

const handleDelete = async () => {
    deleting.value = true;

    try {
        await $fetch(`/api/notes/${data.value.id}`, { method: 'DELETE' });
        $toast.success('Note deleted successfully');
    } catch (error: any) {
        $toast.error(error.data.message);
    }

    deleting.value = false;
};

const handleCopy = () => {
    copy(data.value.content);
    $toast.success('Note copied to clipboard');
};

const changeNote = async (newNote: NoteData) => {
    await tl.value?.play();
    data.value = newNote;
    await tl.value?.reverse();
};

const handlePrev = () => {
    if (!prev.value || !isOpen.value) return;
    changeNote(prev.value);
};

const handleNext = () => {
    if (!next.value || !isOpen.value) return;
    changeNote(next.value);
};

onKeyStroke('ArrowLeft', handlePrev, { eventName: 'keydown' });
onKeyStroke('ArrowRight', handleNext, { eventName: 'keydown' });

watch(
    () => _data,
    (value) => {
        if (value.id === data.value.id) data.value = value;
    },
);

watch(
    notes,
    (value) => {
        if (!value.find((note) => note.id === data.value.id)) isOpen.value = false;
    },
    { immediate: true },
);

watch([isOpen, editModalOpen], ([open, editModalOpen]) => {
    if (!open && !editModalOpen) data.value = _data;
});
</script>
