import type { NoteModel } from '#shared/prisma/models';

export type NoteData = NoteModel;

export const useNotes = () => {
    return useState<NoteData[]>('notes', () => []);
};
