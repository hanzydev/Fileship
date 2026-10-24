export const isOwnedItem = (item: { access?: FolderAccess }) =>
    !item.access || item.access === 'owner';

export const isEditableItem = (item: { access?: FolderAccess }) => item.access !== 'viewer';
