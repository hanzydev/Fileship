export const useNoteActions = () => {
    const route = useRoute();
    const { $toast } = useNuxtApp();
    const { copy } = useClipboard({ legacy: true });
    const { deleteConfirmation, notifyDeleted } = useTrash();

    const copyLink = (note: NoteData) => {
        copy(note.publicUrl!);
        $toast.success('Link copied to clipboard');
    };

    const copyContent = (note: NoteData) => {
        copy(note.content);
        $toast.success('Content copied to clipboard');
    };

    const togglePublic = async (note: NoteData) => {
        const id = $toast.loading(note.public ? 'Making note private...' : 'Making note public...');

        try {
            await $fetch(`/api/notes/${note.id}`, {
                method: 'PATCH',
                body: { public: !note.public },
            });

            $toast.success(note.public ? 'Note is now private' : 'Note is now public', { id });
        } catch (error: any) {
            $toast.error(getErrorMessage(error), { id });
        }
    };

    const remove = async (note: NoteData) => {
        await askConfirmation({
            ...deleteConfirmation(note.title),
            action: async () => {
                try {
                    await $fetch(`/api/notes/${note.id}`, { method: 'DELETE' });
                    notifyDeleted('Note', { notes: [note.id] });

                    if ((route.params as any).id === note.id) navigateTo('/dashboard/notes');
                } catch (error: any) {
                    $toast.error(getErrorMessage(error));
                }
            },
        });
    };

    return { copyLink, copyContent, togglePublic, remove };
};
