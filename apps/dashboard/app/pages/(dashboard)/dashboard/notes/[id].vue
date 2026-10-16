<template>
    <Head>
        <Title>{{ draft.title || $t('notes.newNote') }}</Title>
    </Head>

    <UiCard v-if="!isNew && !note && !isLoading && loadedId !== id" class="flex-1 justify-center">
        <EmptyState
            :icon="IFileX"
            :title="$t('notes.notFound')"
            :description="$t('notes.notFoundDescription')"
        >
            <UiButton variant="outline" as-child>
                <NuxtLink to="/dashboard/notes">{{ $t('notes.backToNotes') }}</NuxtLink>
            </UiButton>
        </EmptyState>
    </UiCard>

    <UiCard v-else-if="loadedId !== id" class="min-w-0 flex-1 gap-4">
        <UiCardContent class="flex items-center justify-between gap-2">
            <UiSkeleton class="h-4 w-32 rounded-full" />
            <UiSkeleton class="h-8 w-28 rounded-full" />
        </UiCardContent>
        <UiCardContent class="flex flex-1 flex-col gap-4">
            <TextBlock custom-tag="div" variant="h3" class="w-full">
                <UiSkeleton class="h-lh w-1/2 rounded-full" />
            </TextBlock>
            <UiSkeleton class="min-h-[60vh] flex-1" />
        </UiCardContent>
    </UiCard>

    <UiCard v-else class="min-w-0 flex-1 gap-4">
        <UiCardContent class="flex flex-wrap items-center gap-2">
            <UiButton variant="ghost" size="icon-sm" class="md:hidden" as-child>
                <NuxtLink to="/dashboard/notes">
                    <IArrowLeft />
                </NuxtLink>
            </UiButton>

            <TextBlock variant="muted" class="mr-auto flex items-center gap-1.5 text-xs">
                <template v-if="saveState === 'saving'">
                    <UiSpinner class="size-3" />
                    {{ $t('notes.saving') }}
                </template>
                <template v-else-if="saveState === 'error'">
                    <ICircleAlert class="size-3.5 text-destructive" />
                    {{ $t('notes.notSaved') }}
                </template>
                <template v-else-if="!isValid">
                    {{ $t('notes.invalidHint') }}
                </template>
                <template v-else-if="!isNew">
                    <ICheck class="size-3.5" />
                    {{ $t('notes.saved') }}
                </template>
            </TextBlock>

            <UiToggleGroup
                type="single"
                variant="outline"
                size="sm"
                :model-value="mode"
                @update:model-value="$event && (mode = $event as never)"
            >
                <UiToggleGroupItem value="edit" :aria-label="$t('notes.mode.edit')">
                    <IPencil />
                </UiToggleGroupItem>
                <UiToggleGroupItem
                    value="split"
                    :aria-label="$t('notes.mode.split')"
                    class="hidden lg:flex"
                >
                    <IColumns2 />
                </UiToggleGroupItem>
                <UiToggleGroupItem value="preview" :aria-label="$t('notes.mode.preview')">
                    <IEye />
                </UiToggleGroupItem>
            </UiToggleGroup>

            <UiDropdownMenu>
                <UiDropdownMenuTrigger as-child>
                    <UiButton variant="ghost" size="icon-sm" :disabled="!note">
                        <IEllipsis />
                    </UiButton>
                </UiDropdownMenuTrigger>
                <UiDropdownMenuContent v-if="note" align="end" class="w-56">
                    <NoteMenuItems :note="note" type="dropdown" in-note />
                </UiDropdownMenuContent>
            </UiDropdownMenu>
        </UiCardContent>

        <UiCardContent class="flex flex-1 flex-col gap-4">
            <input
                v-model="draft.title"
                :placeholder="$t('notes.titlePlaceholder')"
                maxlength="48"
                class="w-full bg-transparent text-2xl font-semibold tracking-tight outline-none placeholder:text-muted-foreground"
            />

            <div :class="cn('grid flex-1 gap-4', mode === 'split' && 'lg:grid-cols-2')">
                <UiTextarea
                    v-if="mode !== 'preview'"
                    v-model="draft.content"
                    :placeholder="$t('notes.contentPlaceholder')"
                    class="min-h-[60vh] resize-none font-mono"
                />
                <MarkdownRenderer
                    v-if="mode !== 'edit'"
                    :content="draft.content || `*${$t('notes.emptyPreview')}*`"
                    class="min-w-0"
                />
            </div>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
import { IFileX } from '#components';

import type { NoteData } from '~/types/notes';

const route = useRoute();
const labels = useBreadcrumbLabels();
const { $toast } = useNuxtApp();

const mode = ref<'edit' | 'split' | 'preview'>('edit');
const current = ref<SaveTarget | null>(null);

let isMounted = true;
let queue = Promise.resolve();

const draft = reactive({ title: '', content: '' });

const id = computed(() => (route.params as any).id as string);
const isNew = computed(() => id.value === 'new');
const loadedId = computed(() => current.value?.id ?? null);

const {
    data: note,
    status,
    refresh,
} = useLazyAsyncData(
    () => `note:${id.value}`,
    () =>
        isNew.value
            ? Promise.resolve(null)
            : $fetch(`/api/notes/${id.value}`).then((data) => reviveDates<NoteData>(data)),
    { server: false },
);

const isLoading = computed(() => status.value === 'pending' || status.value === 'idle');

const isValidBody = (body: typeof draft) => body.title.trim().length >= 3 && !!body.content;

const isValid = computed(() => isValidBody(draft));

const saveState = computed(() =>
    current.value?.pending ? 'saving' : current.value?.failed ? 'error' : 'idle',
);

const persist = async (target: SaveTarget, body: typeof draft) => {
    const json = JSON.stringify(body);

    if (json === target.snapshot) return;

    try {
        if (target.id === 'new') {
            const created = await $fetch('/api/notes', { method: 'POST', body });

            target.id = created.id;

            if (isMounted && current.value === target && id.value === 'new') {
                navigateTo(`/dashboard/notes/${created.id}`, { replace: true });
            }
        } else {
            await $fetch(`/api/notes/${target.id}`, { method: 'PATCH', body });
        }

        target.snapshot = json;
        target.failed = false;
    } catch (error: any) {
        target.failed = true;
        target.queued = target.snapshot;

        const formErrors = error.data?.data?.formErrors;
        $toast.error(
            formErrors?.title?._errors[0] ||
                formErrors?.content?._errors[0] ||
                getErrorMessage(error),
        );
    }
};

const save = () => {
    const target = current.value;
    const body = { ...draft };
    const json = JSON.stringify(body);

    if (!target || !isValidBody(body) || json === target.queued) return;

    target.queued = json;
    target.pending++;

    queue = queue.then(() => persist(target, body)).finally(() => target.pending--);
};

watch(id, (value) => {
    if (value === loadedId.value) return;

    save();
    current.value = null;
});

watch(
    [id, note],
    () => {
        if (loadedId.value === id.value) return;

        if (isNew.value) {
            Object.assign(draft, { title: '', content: '' });
            mode.value = 'edit';
        } else if (note.value?.id === id.value) {
            Object.assign(draft, {
                title: note.value.title,
                content: note.value.content,
            });
            mode.value = 'preview';
        } else return;

        const snapshot = JSON.stringify(draft);

        current.value = { id: id.value, snapshot, queued: snapshot, pending: 0, failed: false };
    },
    { immediate: true },
);

watch(note, (value, oldValue) => {
    if (oldValue && !value && oldValue.id === loadedId.value) {
        current.value!.snapshot = current.value!.queued = JSON.stringify(draft);
    }
    if (value) labels.value = { ...labels.value, [value.id]: value.title };
});

useListHandle({
    resource: 'notes',
    refresh,
    map: (fn) => note.value && (note.value = fn(note.value)),
    remove: (ids) => note.value && ids.includes(note.value.id) && (note.value = null),
});

watchDebounced(draft, save, { debounce: 800, deep: true });

onBeforeUnmount(() => {
    isMounted = false;
    save();
});

interface SaveTarget {
    id: string;
    snapshot: string;
    queued: string;
    pending: number;
    failed: boolean;
}

definePageMeta({
    key: 'note-editor',
});
</script>
