import type { H3Event } from 'h3';

export const userOnly = (event: H3Event) => {
    if (!event.context.user) throw unauthorizedError;
};

export const adminOnly = (event: H3Event) => {
    userOnly(event);
    if (!isAdmin(event.context.user)) throw forbiddenError;
};

export const superAdminOnly = (event: H3Event) => {
    userOnly(event);
    if (!event.context.user!.superAdmin) throw forbiddenError;
};

export const fileUploaderOnly = (event: H3Event) => {
    userOnly(event);
    if (!canUploadFiles(event.context.user)) throw forbiddenError;
};

export const noteTakerOnly = (event: H3Event) => {
    userOnly(event);
    if (!canTakeNotes(event.context.user)) throw forbiddenError;
};
