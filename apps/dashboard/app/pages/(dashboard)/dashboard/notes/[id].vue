<template>
    <Head>
        <Title>{{ draft.title || 'New note' }}</Title>
    </Head>

    <UiCard v-if="!isNew && !note && !isLoading && loadedId !== id" class="flex-1 justify-center">
        <EmptyState :icon="IFileX" title="Note not found" description="It may have been deleted.">
            <UiButton variant="outline" as-child>
                <NuxtLink to="/dashboard/notes">Back to notes</NuxtLink>
            </UiButton>
        </EmptyState>
    </UiCard>

    <UiCard v-else-if="loadedId !== id" class="min-w-0 flex-1 gap-4">
        <UiCardContent class="flex items-center justify-between gap-2">
            <UiSkeleton class="h-4 w-32 rounded-full" />
            <UiSkeleton class="h-8 w-28 rounded-full" />
        </UiCardContent>
        <UiCardContent class="flex flex-1 flex-col gap-4">
            <TextBlock variant="h3" class="w-full">
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
                    Saving...
                </template>
                <template v-else-if="saveState === 'error'">
                    <ICircleAlert class="size-3.5 text-destructive" />
                    Not saved
                </template>
                <template v-else-if="!isValid">
                    Add a title (3+ characters) and some content to save
                </template>
                <template v-else-if="!isNew">
                    <ICheck class="size-3.5" />
                    Saved
                </template>
            </TextBlock>

            <UiToggleGroup
                type="single"
                variant="outline"
                size="sm"
                :model-value="mode"
                @update:model-value="$event && (mode = $event as never)"
            >
                <UiToggleGroupItem value="edit" aria-label="Edit">
                    <IPencil />
                </UiToggleGroupItem>
                <UiToggleGroupItem value="split" aria-label="Split" class="hidden lg:flex">
                    <IColumns2 />
                </UiToggleGroupItem>
                <UiToggleGroupItem value="preview" aria-label="Preview">
                    <IEye />
                </UiToggleGroupItem>
            </UiToggleGroup>

            <UiDropdownMenu>
                <UiDropdownMenuTrigger as-child>
                    <UiButton variant="ghost" size="icon-sm" :disabled="!note">
                        <IEllipsis />
                    </UiButton>
                </UiDropdownMenuTrigger>
                <UiDropdownMenuContent v-if="note" align="end" class="w-52">
                    <NoteMenuItems :note="note" type="dropdown" in-note />
                </UiDropdownMenuContent>
            </UiDropdownMenu>
        </UiCardContent>

        <UiCardContent class="flex flex-1 flex-col gap-4">
            <input
                v-model="draft.title"
                placeholder="Untitled note"
                maxlength="48"
                class="w-full bg-transparent text-2xl font-semibold tracking-tight outline-none placeholder:text-muted-foreground"
            />

            <div :class="cn('grid flex-1 gap-4', mode === 'split' && 'lg:grid-cols-2')">
                <UiTextarea
                    v-if="mode !== 'preview'"
                    v-model="draft.content"
                    placeholder="Write in Markdown..."
                    class="min-h-[60vh] resize-none font-mono"
                />
                <MarkdownRenderer
                    v-if="mode !== 'edit'"
                    :content="draft.content || '*Nothing to preview yet.*'"
                    class="min-w-0"
                />
            </div>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
import { IFileX } from '#components';

const route = useRoute();
const labels = useBreadcrumbLabels();
const { $toast } = useNuxtApp();

const mode = ref<'edit' | 'split' | 'preview'>('edit');
const saveState = ref<'idle' | 'saving' | 'error'>('idle');
const loadedId = ref<string | null>(null);
const snapshot = ref('');

let isMounted = true;

const draft = reactive({ title: '', content: '' });

const id = computed(() => (route.params as any).id as string);
const isNew = computed(() => id.value === 'new');

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

const isValid = computed(() => draft.title.trim().length >= 3 && !!draft.content);
const isDirty = computed(() => JSON.stringify(draft) !== snapshot.value);

const save = async () => {
    const targetId = loadedId.value;

    if (!targetId || !isValid.value || !isDirty.value || saveState.value === 'saving') return;

    const body = { ...draft };
    const isCurrent = () => loadedId.value === targetId;

    saveState.value = 'saving';

    try {
        if (targetId === 'new') {
            const created = await $fetch('/api/notes', { method: 'POST', body });

            if (isCurrent()) {
                loadedId.value = created.id;
                snapshot.value = JSON.stringify(body);
            }

            if (isMounted && id.value === 'new') {
                await navigateTo(`/dashboard/notes/${created.id}`, { replace: true });
            }
        } else {
            await $fetch(`/api/notes/${targetId}`, { method: 'PATCH', body });
            if (isCurrent()) snapshot.value = JSON.stringify(body);
        }

        saveState.value = 'idle';

        if (isDirty.value) save();
    } catch (error: any) {
        saveState.value = 'error';

        const formErrors = error.data?.data?.formErrors;
        $toast.error(
            formErrors?.title?._errors[0] ||
                formErrors?.content?._errors[0] ||
                getErrorMessage(error),
        );
    }
};

watch(id, (value) => {
    if (value === loadedId.value) return;

    save();
    loadedId.value = null;
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

        loadedId.value = id.value;
        snapshot.value = JSON.stringify(draft);
        saveState.value = 'idle';
    },
    { immediate: true },
);

watch(note, (value, oldValue) => {
    if (oldValue && !value && oldValue.id === loadedId.value)
        snapshot.value = JSON.stringify(draft);
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

definePageMeta({
    key: 'note-editor',
});
</script>
