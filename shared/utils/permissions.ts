import type { H3Event } from 'h3';

import { UserPermission } from '@prisma/client';

export type User = {
    permissions: UserPermission[];
    superAdmin: boolean;
} | null;

type UserOrEvent = MaybeRef<User> | H3Event;

const hasPermission = (user: UserOrEvent, permission: UserPermission) => {
    if (!user) return false;

    const _user = 'context' in user ? user.context.user : 'value' in user ? user.value : user;
    if (!_user) return false;

    return (
        _user?.superAdmin ||
        _user?.permissions?.includes(UserPermission.Admin) ||
        _user?.permissions?.includes(permission)
    );
};

export const isAdmin = (user: UserOrEvent) => hasPermission(user, UserPermission.Admin);

export const canUploadFiles = (user: UserOrEvent) =>
    hasPermission(user, UserPermission.UploadFiles);

export const canTakeNotes = (user: UserOrEvent) => hasPermission(user, UserPermission.TakeNotes);

export const canShareCodes = (user: UserOrEvent) => hasPermission(user, UserPermission.ShareCodes);

export const canShortenUrls = (user: UserOrEvent) =>
    hasPermission(user, UserPermission.ShortenUrls);

export enum PermissionIcon {
    Admin = 'heroicons-solid:shield-check',
    UploadFiles = 'heroicons-solid:upload',
    TakeNotes = 'heroicons:pencil-16-solid',
    ShareCodes = 'heroicons-solid:code',
    ShortenUrls = 'heroicons:link-16-solid',
}
