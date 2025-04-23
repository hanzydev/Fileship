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
                icon="heroicons-solid:arrow-narrow-left"
                icon-class="text-2xl! sm:text-3xl!"
                rounded-full="!"
                p0="!"
                shadow="xl fs-overlay-1"
                aria-label="Previous"
                top="1/2"
                translate-y-="1/2"
                data-controller
                absolute
                sm="left-6"
                left-3
                z60
                size="10 sm:14"
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
                icon="heroicons-solid:arrow-narrow-right"
                icon-class="text-2xl! sm:text-3xl!"
                rounded-full="!"
                p0="!"
                shadow="xl fs-overlay-1"
                aria-label="Previous"
                top="1/2"
                translate-y-="1/2"
                data-controller
                absolute
                sm="right-6"
                right-3
                z60
                size="10 sm:14"
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

        <UiTextArea v-model="data!.content" label="Content" readonly wfull cursor-text="!" />

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
import { Cubic, gsap } from 'gsap';
import { toast } from 'vue-sonner';

const { data: _data } = defineProps<{
    data: NoteData;
}>();

const isOpen = defineModel<boolean>({ required: true });

const data = ref(_data);

const notes = useNotes();
const currentUser = useAuthUser();
const { copied, copy } = useClipboard({ legacy: true });

const modalId = useId();
const reducedMotion = usePreferredReducedMotion();

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
    await $fetch(`/api/notes/${data.value.id}`, { method: 'DELETE' });
    deleting.value = false;

    toast.success('Note deleted successfully');
};

const handleCopy = () => {
    copy(data.value.content);
    toast.success('Note copied to clipboard');
};

const handlePrev = async () => {
    if (!prev.value) return;

    await tl.value?.play();
    data.value = prev.value!;
    await tl.value?.reverse();
};

const handleNext = async () => {
    if (!next.value) return;

    await tl.value?.play();
    data.value = next.value!;
    await tl.value?.reverse();
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
