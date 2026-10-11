export const useNoteActions = () => {
    const route = useRoute();
    const { $toast } = useNuxtApp();
    const { copy } = useClipboard({ legacy: true });

    const copyLink = (note: NoteData) => {
        copy(note.publicUrl!);
        $toast.success('Link copied to clipboard');
    };

    const copyContent = (note: NoteData) => {
        copy(note.content);
        $toast.success('Content copied to clipboard');
    };

    const togglePublic = async (note: NoteData) => {
        try {
            await $fetch(`/api/notes/${note.id}`, {
                method: 'PATCH',
                body: { public: !note.public },
            });

            $toast.success(note.public ? 'Note is now private' : 'Note is now public');
        } catch (error: any) {
            $toast.error(error.data.message);
        }
    };

    const remove = async (note: NoteData) => {
        const confirmed = await askConfirmation({
            title: `Delete ${note.title}?`,
            description: 'This action cannot be undone.',
            confirmText: 'Delete',
            destructive: true,
        });

        if (!confirmed) return;

        try {
            await $fetch(`/api/notes/${note.id}`, { method: 'DELETE' });
            $toast.success('Note deleted');

            if ((route.params as any).id === note.id) navigateTo('/dashboard/notes');
        } catch (error: any) {
            $toast.error(error.data.message);
        }
    };

    return { copyLink, copyContent, togglePublic, remove };
};
