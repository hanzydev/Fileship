import type { NoteModel } from '#shared/prisma/models';

export type NoteData = NoteModel & { publicUrl?: string | null };
