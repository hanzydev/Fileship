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

            <UiTabs :model-value="mode" @update:model-value="$event && (mode = $event as never)">
                <UiTabsList>
                    <UiTabsTrigger value="edit" aria-label="Edit">
                        <IPencil />
                    </UiTabsTrigger>
                    <UiTabsTrigger value="split" aria-label="Split" class="hidden lg:flex">
                        <IColumns2 />
                    </UiTabsTrigger>
                    <UiTabsTrigger value="preview" aria-label="Preview">
                        <IEye />
                    </UiTabsTrigger>
                </UiTabsList>
            </UiTabs>

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
                class="w-full bg-transparent text-2xl font-bold tracking-tight outline-none placeholder:text-muted-foreground"
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
const notes = useNotes();
const { $toast } = useNuxtApp();
const { status } = useLazyAsyncData('notes', fetchNotes, { server: false });

const mode = ref<'edit' | 'split' | 'preview'>('edit');
const saveState = ref<'idle' | 'saving' | 'error'>('idle');
const loadedId = ref<string | null>(null);
const snapshot = ref('');

const draft = reactive({ title: '', content: '' });

const id = computed(() => (route.params as any).id as string);
const isNew = computed(() => id.value === 'new');
const note = computed(() => notes.value.find((n) => n.id === id.value));
const isLoading = computed(() => status.value !== 'success' && !notes.value.length);

const isValid = computed(() => draft.title.trim().length >= 3 && !!draft.content);
const isDirty = computed(() => JSON.stringify(draft) !== snapshot.value);

const save = async () => {
    if (!isValid.value || !isDirty.value || saveState.value === 'saving') return;

    const body = { ...draft };
    saveState.value = 'saving';

    try {
        if (isNew.value) {
            const created = await $fetch('/api/notes', { method: 'POST', body });

            loadedId.value = created.id;
            await navigateTo(`/dashboard/notes/${created.id}`, { replace: true });
        } else {
            await $fetch(`/api/notes/${id.value}`, { method: 'PATCH', body });
        }

        snapshot.value = JSON.stringify(body);
        saveState.value = 'idle';

        if (isDirty.value) save();
    } catch (error: any) {
        saveState.value = 'error';

        const formErrors = error.data.data?.formErrors;
        $toast.error(
            formErrors?.title?._errors[0] || formErrors?.content?._errors[0] || error.data.message,
        );
    }
};

watch(
    [id, note],
    () => {
        if (loadedId.value === id.value) return;

        if (isNew.value) {
            Object.assign(draft, { title: '', content: '' });
            mode.value = 'edit';
        } else if (note.value) {
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
    if (oldValue && !value) snapshot.value = JSON.stringify(draft);
});

watchDebounced(draft, save, { debounce: 800, deep: true });

onBeforeUnmount(save);

definePageMeta({
    key: 'note-editor',
});
</script>
