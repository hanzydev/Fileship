import type { NoteData } from '~/types/notes';

export const useNoteActions = () => {
    const route = useRoute();
    const { $toast, $i18n } = useNuxtApp();
    const { t } = $i18n;
    const { copy } = useClipboard({ legacy: true });
    const { deleteConfirmation, notifyDeleted } = useTrash();

    const copyLink = (note: NoteData) => {
        copy(note.publicUrl!);
        $toast.success(t('actions.linkCopied'));
    };

    const copyContent = (note: NoteData) => {
        copy(note.content);
        $toast.success(t('actions.contentCopied'));
    };

    const togglePublic = async (note: NoteData) => {
        const id = $toast.loading(
            t(note.public ? 'actions.note.makingPrivate' : 'actions.note.makingPublic'),
        );

        try {
            await $fetch(`/api/notes/${note.id}`, {
                method: 'PATCH',
                body: { public: !note.public },
            });

            $toast.success(t(note.public ? 'actions.note.nowPrivate' : 'actions.note.nowPublic'), {
                id,
            });
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
                    notifyDeleted(t('actions.note.label'), { notes: [note.id] });

                    if ((route.params as any).id === note.id) navigateTo('/dashboard/notes');
                } catch (error: any) {
                    $toast.error(getErrorMessage(error));
                }
            },
        });
    };

    return { copyLink, copyContent, togglePublic, remove };
};
