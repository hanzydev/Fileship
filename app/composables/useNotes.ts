import type { Note } from '@prisma/client';

export type NoteData = Note;

export const useNotes = () => {
    return useState<NoteData[]>('notes', () => []);
};
