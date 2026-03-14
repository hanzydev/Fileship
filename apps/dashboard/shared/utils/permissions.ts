import { H3Event } from 'h3';
import { type MaybeRef, unref } from 'vue';

import { UserPermission } from '#shared/prisma/enums';

export type PartialUser = {
    permissions: UserPermission[];
    superAdmin: boolean;
} | null;

type UserOrEvent = MaybeRef<PartialUser> | H3Event;

const hasPermission = (user: UserOrEvent, permission: UserPermission) => {
    if (!user) return false;

    const _user = user instanceof H3Event ? user.context.user : unref(user);
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

export enum PermissionIcon {
    Admin = 'solar:shield-check-bold',
    UploadFiles = 'solar:upload-minimalistic-linear',
    TakeNotes = 'solar:pen-2-bold',
}
