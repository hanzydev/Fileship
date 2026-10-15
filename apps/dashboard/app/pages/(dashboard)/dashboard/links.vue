<template>
    <Head>
        <Title>Links</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader
            title="Links"
            :description="isLoading ? 'Loading...' : pluralize(total, 'short link')"
        >
            <UiButton @click="handleOpen()">
                <IPlus />
                Shorten link
            </UiButton>
        </PageHeader>

        <UiInputGroup class="max-w-sm">
            <UiInputGroupAddon>
                <UiSpinner v-if="!!search && isFetching" />
                <ISearch v-else />
            </UiInputGroupAddon>
            <UiInputGroupInput v-model="search" placeholder="Search links..." />
        </UiInputGroup>

        <EmptyState
            v-if="isError"
            :icon="ICircleAlert"
            title="Couldn't load short links"
            description="Something went wrong. Check your connection and try again."
        >
            <UiButton variant="outline" @click="refresh">Try again</UiButton>
        </EmptyState>

        <EmptyState
            v-else-if="!isLoading && !links.length"
            :icon="ILink"
            :title="search ? 'No matching links' : 'No short links yet'"
            :description="
                search ? 'Try a different search.' : 'Shorten a link to share it with a short URL.'
            "
        >
            <UiButton v-if="!search" @click="handleOpen()">
                <IPlus />
                Shorten link
            </UiButton>
        </EmptyState>

        <DataTable
            v-else
            v-model:sort="sort"
            :items="links"
            :item-key="(link) => link.id"
            :columns="columns"
            :card-estimate-size="74"
            :loading="isLoading"
            :loading-more="isLoadingMore"
            @reach-end="loadMore"
        >
            <template #cell-code="{ item }">
                <div class="flex min-w-0 items-center gap-2">
                    <span class="truncate font-medium">/s/{{ item.code }}</span>
                    <ILock v-if="item.password" class="size-3.5 shrink-0 text-muted-foreground" />
                </div>
            </template>
            <template #cell-url="{ item }">
                <span class="block truncate text-muted-foreground" :title="item.url">
                    {{ item.url }}
                </span>
            </template>
            <template #cell-clicks="{ item }">
                {{ item.maxClicks ? `${item.clicks} / ${item.maxClicks}` : item.clicks }}
            </template>
            <template #cell-expiresAt="{ item }">
                <span class="block truncate text-muted-foreground">
                    {{ item.expiresAt ? dayjs(item.expiresAt).fromNow() : 'Never' }}
                </span>
            </template>
            <template #cell-createdAt="{ item }">
                <span class="block truncate text-muted-foreground">
                    {{ dayjs(item.createdAt).fromNow() }}
                </span>
            </template>
            <template #cell-actions="{ item }">
                <div class="flex items-center justify-end gap-1">
                    <UiButton
                        variant="ghost"
                        size="icon-sm"
                        aria-label="Copy link"
                        @click="handleCopy(item)"
                    >
                        <ICopy />
                    </UiButton>
                    <UiDropdownMenu>
                        <UiDropdownMenuTrigger as-child>
                            <UiButton variant="ghost" size="icon-sm">
                                <IEllipsis />
                            </UiButton>
                        </UiDropdownMenuTrigger>
                        <UiDropdownMenuContent align="end" class="w-56">
                            <UiDropdownMenuItem as-child>
                                <a :href="item.url" target="_blank" rel="noopener noreferrer">
                                    <IExternalLink />
                                    Open target
                                </a>
                            </UiDropdownMenuItem>
                            <UiDropdownMenuItem @select="handleOpen(item)">
                                <IPencil />
                                Edit
                            </UiDropdownMenuItem>
                            <UiDropdownMenuSeparator />
                            <UiDropdownMenuItem variant="destructive" @select="handleDelete(item)">
                                <ITrash2 />
                                Delete
                            </UiDropdownMenuItem>
                        </UiDropdownMenuContent>
                    </UiDropdownMenu>
                </div>
            </template>
            <template #card-skeleton>
                <ListCardSkeleton media="size-4" />
            </template>
            <template #card="{ item }">
                <UiCard size="sm">
                    <UiCardContent class="flex min-h-8 items-center gap-3">
                        <ILink class="size-4 shrink-0 text-muted-foreground" />
                        <div class="flex min-w-0 flex-1 flex-col">
                            <TextBlock variant="small" class="truncate">
                                /s/{{ item.code }}
                            </TextBlock>
                            <TextBlock variant="muted" class="truncate text-xs">
                                {{ pluralize(item.clicks, 'click') }} · {{ item.url }}
                            </TextBlock>
                        </div>
                        <UiButton
                            variant="ghost"
                            size="icon-sm"
                            aria-label="Copy link"
                            @click="handleCopy(item)"
                        >
                            <ICopy />
                        </UiButton>
                        <UiButton
                            variant="ghost"
                            size="icon-sm"
                            aria-label="Edit link"
                            @click="handleOpen(item)"
                        >
                            <IPencil />
                        </UiButton>
                    </UiCardContent>
                </UiCard>
            </template>
        </DataTable>
    </div>

    <UiDialog v-model:open="dialog.open">
        <UiDialogContent class="sm:max-w-md">
            <form class="flex flex-col gap-6" @submit.prevent="handleSave">
                <UiDialogHeader>
                    <UiDialogTitle>{{ dialog.link ? 'Edit link' : 'Shorten link' }}</UiDialogTitle>
                    <UiDialogDescription>
                        {{
                            dialog.link
                                ? 'Changes apply to the short link right away.'
                                : 'The short link is copied to your clipboard when it is created.'
                        }}
                    </UiDialogDescription>
                </UiDialogHeader>

                <UiFieldGroup class="grid gap-x-4 sm:grid-cols-2">
                    <UiField class="sm:col-span-2" :data-invalid="!!formErrors?.url">
                        <UiFieldLabel for="link-url">URL</UiFieldLabel>
                        <UiInput
                            id="link-url"
                            v-model="form.url"
                            placeholder="https://example.com/a/very/long/link"
                            autofocus
                        />
                        <UiFieldError v-if="formErrors?.url" :errors="formErrors.url._errors" />
                    </UiField>

                    <UiField class="sm:col-span-2" :data-invalid="!!formErrors?.code">
                        <UiFieldLabel for="link-code">Short link</UiFieldLabel>
                        <UiInputGroup>
                            <UiInputGroupAddon>
                                <UiInputGroupText>/s/</UiInputGroupText>
                            </UiInputGroupAddon>
                            <UiInputGroupInput
                                id="link-code"
                                v-model="form.code"
                                placeholder="Random"
                            />
                        </UiInputGroup>
                        <UiFieldError v-if="formErrors?.code" :errors="formErrors.code._errors" />
                    </UiField>

                    <UiField>
                        <UiFieldLabel for="link-password">Password</UiFieldLabel>
                        <PasswordInput
                            id="link-password"
                            v-model="form.password"
                            placeholder="Optional"
                            autocomplete="off"
                        />
                    </UiField>

                    <UiField>
                        <UiFieldLabel for="link-max-clicks">Max clicks</UiFieldLabel>
                        <UiNumberField id="link-max-clicks" v-model="form.maxClicks" :min="0">
                            <UiNumberFieldContent>
                                <UiNumberFieldDecrement />
                                <UiNumberFieldInput />
                                <UiNumberFieldIncrement />
                            </UiNumberFieldContent>
                        </UiNumberField>
                    </UiField>

                    <UiField class="sm:col-span-2">
                        <UiFieldLabel>Expiration</UiFieldLabel>
                        <UiSelect v-model="form.expiration">
                            <UiSelectTrigger class="w-full">
                                <UiSelectValue />
                            </UiSelectTrigger>
                            <UiSelectContent>
                                <UiSelectItem
                                    v-for="option in EXPIRATIONS"
                                    :key="option.label"
                                    :value="option.label"
                                >
                                    {{ option.label }}
                                </UiSelectItem>
                            </UiSelectContent>
                        </UiSelect>
                        <UiFieldDescription>
                            The link stops working after this time or once it reaches its max
                            clicks.
                        </UiFieldDescription>
                    </UiField>
                </UiFieldGroup>

                <UiDialogFooter>
                    <UiButton type="button" variant="outline" @click="dialog.open = false">
                        Cancel
                    </UiButton>
                    <UiButton type="submit" :disabled="isSaving || !form.url">
                        <UiSpinner v-if="isSaving" />
                        {{ dialog.link ? 'Save' : 'Shorten' }}
                    </UiButton>
                </UiDialogFooter>
            </form>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

import { ICircleAlert, ILink } from '#components';

import type { LinkData } from '~/types/links';

const route = useRoute();
const { $toast } = useNuxtApp();
const { copy } = useClipboard({ legacy: true });

const search = useQueryState('q', '');
const sort = ref({ key: 'createdAt', desc: true });
const formErrors = ref();
const isSaving = ref(false);

const dialog = reactive({ open: false, link: null as LinkData | null });

const form = reactive({
    url: '',
    code: '',
    password: '',
    maxClicks: 0,
    expiration: 'Never',
});

const columns = [
    { key: 'code', label: 'Short link', sortable: true, class: 'w-48' },
    { key: 'url', label: 'Target' },
    { key: 'clicks', label: 'Clicks', sortable: true, descFirst: true, class: 'w-24' },
    { key: 'expiresAt', label: 'Expires', class: 'hidden w-32 lg:table-cell' },
    {
        key: 'createdAt',
        label: 'Created',
        sortable: true,
        descFirst: true,
        class: 'hidden w-32 xl:table-cell',
    },
    { key: 'actions', label: '', class: 'w-24' },
];

const {
    items: links,
    total,
    isLoading,
    isError,
    isFetching,
    isLoadingMore,
    loadMore,
    refresh,
} = usePaginatedList<LinkData>('links', '/api/links', () => ({
    q: search.value.trim(),
    sort: `${sort.value.key}:${sort.value.desc ? 'desc' : 'asc'}`,
}));

const handleCopy = (link: LinkData) => {
    copy(link.shortUrl);
    $toast.success('Link copied to clipboard');
};

const handleOpen = (link?: LinkData) => {
    formErrors.value = null;
    dialog.link = link ?? null;

    Object.assign(form, {
        url: link?.url ?? '',
        code: link?.code ?? '',
        password: link?.password ?? '',
        maxClicks: link?.maxClicks ?? 0,
        expiration: predictExpiration(link?.expiresAt ?? null),
    });

    dialog.open = true;
};

const handleSave = async () => {
    isSaving.value = true;
    formErrors.value = null;

    const body = {
        url: form.url.trim(),
        code: form.code.trim() || (dialog.link ? undefined : null),
        password: form.password || null,
        maxClicks: form.maxClicks,
        ...((!dialog.link || form.expiration !== predictExpiration(dialog.link.expiresAt)) && {
            expiration: EXPIRATIONS.find((option) => option.label === form.expiration)!.value,
        }),
    };

    try {
        const link = dialog.link
            ? await $fetch<LinkData>(`/api/links/${dialog.link.id}`, { method: 'PATCH', body })
            : await $fetch<LinkData>('/api/links', { method: 'POST', body });

        dialog.open = false;

        if (dialog.link) {
            $toast.success('Link saved');
        } else {
            copy(link.shortUrl);
            $toast.success('Short link copied to clipboard');
        }
    } catch (error: any) {
        formErrors.value = error.data?.data?.formErrors;
        if (!formErrors.value) $toast.error(getErrorMessage(error));
    }

    isSaving.value = false;
};

const handleDelete = async (link: LinkData) => {
    await askConfirmation({
        title: `Delete /s/${link.code}?`,
        description: 'The short link stops working right away.',
        confirmText: 'Delete',
        destructive: true,
        action: async () => {
            try {
                await $fetch(`/api/links/${link.id}`, { method: 'DELETE' });
                $toast.success('Link deleted');
            } catch (error: any) {
                $toast.error(getErrorMessage(error));
                return false;
            }
        },
    });
};

onMounted(() => {
    if (!route.query.new) return;

    handleOpen();
    navigateTo({ query: {} }, { replace: true });
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'link-shortener-only',
});
</script>
