<template>
    <UiCommandDialog v-model:open="open" title="Search" description="Search or run a command">
        <UiCommandInput
            placeholder="Search files, folders, notes, links or commands..."
            @update:model-value="query = $event"
        />
        <UiCommandList class="mb-1">
            <UiCommandEmpty>No results found.</UiCommandEmpty>

            <UiCommandGroup v-if="term && matchedFiles.length" class="px-1 pb-0" heading="Files">
                <UiCommandItem
                    v-for="file in matchedFiles"
                    :key="`${file.id}:${term}`"
                    :value="`file:${file.id}`"
                    @select="handleSelect(`/dashboard/files?file=${file.id}`)"
                >
                    <IFile />
                    <span class="truncate">{{ file.fileName }}</span>
                    <UiCommandShortcut>{{ file.size.formatted }}</UiCommandShortcut>
                </UiCommandItem>
            </UiCommandGroup>

            <UiCommandGroup
                v-if="term && matchedFolders.length"
                class="px-1 pb-0"
                heading="Folders"
            >
                <UiCommandItem
                    v-for="folder in matchedFolders"
                    :key="`${folder.id}:${term}`"
                    :value="`folder:${folder.id}`"
                    @select="handleSelect(`/dashboard/folders/${folder.id}`)"
                >
                    <IFolder />
                    <span class="truncate">{{ folder.name }}</span>
                </UiCommandItem>
            </UiCommandGroup>

            <UiCommandGroup v-if="term && matchedNotes.length" class="px-1 pb-0" heading="Notes">
                <UiCommandItem
                    v-for="note in matchedNotes"
                    :key="`${note.id}:${term}`"
                    :value="`note:${note.id}`"
                    @select="handleSelect(`/dashboard/notes/${note.id}`)"
                >
                    <IBookmark />
                    <span class="truncate">{{ note.title }}</span>
                </UiCommandItem>
            </UiCommandGroup>

            <UiCommandGroup v-if="term && matchedLinks.length" class="px-1 pb-0" heading="Links">
                <UiCommandItem
                    v-for="link in matchedLinks"
                    :key="`${link.id}:${term}`"
                    :value="`link:${link.id}`"
                    @select="handleSelect(`/dashboard/links?q=${encodeURIComponent(link.code)}`)"
                >
                    <ILink />
                    <span class="truncate">{{ link.code }}</span>
                    <UiCommandShortcut class="max-w-48 truncate">{{ link.url }}</UiCommandShortcut>
                </UiCommandItem>
            </UiCommandGroup>

            <UiCommandGroup v-if="term && matchedUsers.length" class="px-1 pb-0" heading="Users">
                <UiCommandItem
                    v-for="user in matchedUsers"
                    :key="`${user.id}:${term}`"
                    :value="`user:${user.id}`"
                    @select="handleSelect(`/dashboard/users?user=${user.id}`)"
                >
                    <IUser />
                    <span class="truncate">{{ user.username }}</span>
                </UiCommandItem>
            </UiCommandGroup>

            <UiCommandGroup class="px-1 pb-0" heading="Actions">
                <UiCommandItem
                    v-for="action in actions"
                    :key="action.label"
                    :value="action.label"
                    @select="handleAction(action.run)"
                >
                    <component :is="action.icon" />
                    {{ action.label }}
                    <UiCommandShortcut v-if="action.shortcut">
                        {{ action.shortcut }}
                    </UiCommandShortcut>
                </UiCommandItem>
            </UiCommandGroup>

            <UiCommandSeparator />

            <UiCommandGroup class="px-1 pb-0" heading="Go to">
                <UiCommandItem
                    v-for="page in pages"
                    :key="page.url"
                    :value="page.label"
                    @select="handleSelect(page.url)"
                >
                    <component :is="page.icon" />
                    {{ page.label }}
                </UiCommandItem>
            </UiCommandGroup>
        </UiCommandList>
    </UiCommandDialog>
</template>

<script setup lang="ts">
import {
    IBookmark,
    IChartNoAxesColumn,
    IClipboardList,
    IFiles,
    IFolder,
    IFolderPlus,
    IHome,
    ILink,
    IMoon,
    IServer,
    ISettings,
    ISquarePen,
    ISun,
    ITrash2,
    IUpload,
    IUsers,
} from '#components';

const open = defineModel<boolean>('open', { required: true });

const authUser = useAuthUser();
const trash = useTrash();
const colorMode = useColorMode();
const uploadSheetOpen = useUploadSheet();

const query = ref('');

const term = computed(() => query.value.trim());

const useMatches = <T extends { id: string }>(
    resource: ListResource,
    endpoint: string,
    isAllowed: () => boolean,
) => {
    const { items, isFresh } = usePaginatedList<T>(resource, endpoint, () => ({ q: term.value }), {
        limit: 5,
        enabled: () => open.value && !!term.value && isAllowed(),
    });

    return computed(() => (isFresh.value ? items.value : []));
};

const matchedFiles = useMatches<FileData>('files', '/api/files', () =>
    canUploadFiles(authUser.value),
);

const matchedFolders = useMatches<FolderData>('folders', '/api/folders', () =>
    canUploadFiles(authUser.value),
);

const matchedNotes = useMatches<NoteData>('notes', '/api/notes', () =>
    canTakeNotes(authUser.value),
);

const matchedLinks = useMatches<LinkData>('links', '/api/links', () =>
    canShortenLinks(authUser.value),
);

const matchedUsers = useMatches<UserData>('users', '/api/users', () => isAdmin(authUser.value));

const pages = computed(() => [
    { label: 'Home', url: '/dashboard', icon: IHome },
    ...(canUploadFiles(authUser.value)
        ? [
              { label: 'Files', url: '/dashboard/files', icon: IFiles },
              { label: 'Folders', url: '/dashboard/folders', icon: IFolder },
          ]
        : []),
    ...(canTakeNotes(authUser.value)
        ? [{ label: 'Notes', url: '/dashboard/notes', icon: IBookmark }]
        : []),
    ...(canShortenLinks(authUser.value)
        ? [{ label: 'Links', url: '/dashboard/links', icon: ILink }]
        : []),
    ...(trash.isEnabled.value && (canUploadFiles(authUser.value) || canTakeNotes(authUser.value))
        ? [{ label: 'Trash', url: '/dashboard/trash', icon: ITrash2 }]
        : []),
    ...(isAdmin(authUser.value)
        ? [
              { label: 'Statistics', url: '/dashboard/stats', icon: IChartNoAxesColumn },
              { label: 'Users', url: '/dashboard/users', icon: IUsers },
              { label: 'Logs', url: '/dashboard/logs', icon: IClipboardList },
              ...(authUser.value?.superAdmin
                  ? [{ label: 'Instance', url: '/dashboard/instance', icon: IServer }]
                  : []),
          ]
        : []),
    { label: 'Settings', url: '/dashboard/settings/profile', icon: ISettings },
]);

const actions = computed(() => [
    ...(canUploadFiles(authUser.value)
        ? [
              {
                  label: 'Upload files',
                  icon: IUpload,
                  shortcut: 'U',
                  run: () => (uploadSheetOpen.value = true),
              },
              {
                  label: 'New folder',
                  icon: IFolderPlus,
                  run: () => navigateTo('/dashboard/folders?new=1'),
              },
          ]
        : []),
    ...(canTakeNotes(authUser.value)
        ? [{ label: 'New note', icon: ISquarePen, run: () => navigateTo('/dashboard/notes/new') }]
        : []),
    ...(canShortenLinks(authUser.value)
        ? [
              {
                  label: 'Shorten link',
                  icon: ILink,
                  run: () => navigateTo('/dashboard/links?new=1'),
              },
          ]
        : []),
    {
        label: colorMode.value === 'dark' ? 'Switch to light mode' : 'Switch to dark mode',
        icon: colorMode.value === 'dark' ? ISun : IMoon,
        run: () => (colorMode.preference = colorMode.value === 'dark' ? 'light' : 'dark'),
    },
]);

const handleSelect = (url: string) => {
    open.value = false;
    navigateTo(url);
};

const handleAction = (run: () => unknown) => {
    open.value = false;
    run();
};

watch(open, (value) => value && (query.value = ''));

onKeyStroke(['k', 'K'], (event) => {
    if (event.ctrlKey || event.metaKey) {
        event.preventDefault();
        open.value = !open.value;
    }
});
</script>
