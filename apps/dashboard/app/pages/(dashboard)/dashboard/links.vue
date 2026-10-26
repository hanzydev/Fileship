<template>
    <Head>
        <Title>{{ $t('links.title') }}</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader
            :title="$t('links.title')"
            :description="isLoading ? $t('common.loading') : $t('links.count', total)"
        >
            <UiButton @click="handleOpen()">
                <IPlus />
                {{ $t('links.shorten') }}
            </UiButton>
        </PageHeader>

        <UiInputGroup class="max-w-sm">
            <UiInputGroupAddon>
                <UiSpinner v-if="!!search && isFetching" />
                <ISearch v-else />
            </UiInputGroupAddon>
            <UiInputGroupInput v-model="search" :placeholder="$t('links.searchPlaceholder')" />
        </UiInputGroup>

        <EmptyState
            v-if="isError"
            :icon="ICircleAlert"
            :title="$t('links.loadFailed')"
            :description="$t('links.loadFailedDescription')"
        >
            <UiButton variant="outline" @click="refresh">{{ $t('links.tryAgain') }}</UiButton>
        </EmptyState>

        <EmptyState
            v-else-if="!isLoading && !links.length"
            :icon="ILink"
            :title="search ? $t('links.noMatches') : $t('links.empty')"
            :description="search ? $t('links.noMatchesDescription') : $t('links.emptyDescription')"
        >
            <UiButton v-if="!search" @click="handleOpen()">
                <IPlus />
                {{ $t('links.shorten') }}
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
                    <span class="truncate font-medium">
                        {{ $t('links.codePrefix') }}{{ item.code }}
                    </span>
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
                    {{ item.expiresAt ? date(item.expiresAt).fromNow() : $t('links.never') }}
                </span>
            </template>
            <template #cell-createdAt="{ item }">
                <span class="block truncate text-muted-foreground">
                    {{ date(item.createdAt).fromNow() }}
                </span>
            </template>
            <template #cell-actions="{ item }">
                <div class="flex items-center justify-end gap-1">
                    <UiButton
                        variant="ghost"
                        size="icon-sm"
                        :aria-label="$t('links.copy')"
                        @click="handleCopy(item)"
                    >
                        <ICopy />
                    </UiButton>
                    <LinkMenu :link="item" @edit="handleOpen(item)" @delete="handleDelete(item)" />
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
                                {{ $t('links.codePrefix') }}{{ item.code }}
                            </TextBlock>
                            <TextBlock variant="muted" class="truncate text-xs">
                                {{ $t('links.clickCount', item.clicks) }} · {{ item.url }}
                            </TextBlock>
                        </div>
                        <UiButton
                            variant="ghost"
                            size="icon-sm"
                            :aria-label="$t('links.copy')"
                            @click="handleCopy(item)"
                        >
                            <ICopy />
                        </UiButton>
                        <LinkMenu
                            :link="item"
                            @edit="handleOpen(item)"
                            @delete="handleDelete(item)"
                        />
                    </UiCardContent>
                </UiCard>
            </template>
        </DataTable>
    </div>

    <UiDialog v-model:open="dialog.open">
        <UiDialogContent class="sm:max-w-md">
            <form class="flex flex-col gap-6" @submit.prevent="handleSave">
                <UiDialogHeader>
                    <UiDialogTitle>
                        {{ dialog.link ? $t('links.editLink') : $t('links.shorten') }}
                    </UiDialogTitle>
                    <UiDialogDescription>
                        {{
                            dialog.link
                                ? $t('links.editDescription')
                                : $t('links.createDescription')
                        }}
                    </UiDialogDescription>
                </UiDialogHeader>

                <UiFieldGroup class="grid gap-x-4 sm:grid-cols-2">
                    <UiField class="sm:col-span-2" :data-invalid="!!formErrors?.url">
                        <UiFieldLabel for="link-url">{{ $t('links.url') }}</UiFieldLabel>
                        <UiInput
                            id="link-url"
                            v-model="form.url"
                            :placeholder="$t('links.urlPlaceholder')"
                            autofocus
                        />
                        <UiFieldError v-if="formErrors?.url" :errors="formErrors.url._errors" />
                    </UiField>

                    <UiField class="sm:col-span-2" :data-invalid="!!formErrors?.code">
                        <UiFieldLabel for="link-code">{{ $t('links.shortLink') }}</UiFieldLabel>
                        <UiInputGroup>
                            <UiInputGroupAddon>
                                <UiInputGroupText>{{ $t('links.codePrefix') }}</UiInputGroupText>
                            </UiInputGroupAddon>
                            <UiInputGroupInput
                                id="link-code"
                                v-model="form.code"
                                :placeholder="$t('links.random')"
                            />
                        </UiInputGroup>
                        <UiFieldError v-if="formErrors?.code" :errors="formErrors.code._errors" />
                    </UiField>

                    <UiField>
                        <UiFieldLabel for="link-password">{{ $t('links.password') }}</UiFieldLabel>
                        <PasswordInput
                            id="link-password"
                            v-model="form.password"
                            :placeholder="$t('links.optional')"
                            autocomplete="off"
                        />
                    </UiField>

                    <UiField>
                        <UiFieldLabel for="link-max-clicks">
                            {{ $t('links.maxClicks') }}
                        </UiFieldLabel>
                        <UiNumberField id="link-max-clicks" v-model="form.maxClicks" :min="0">
                            <UiNumberFieldContent>
                                <UiNumberFieldDecrement />
                                <UiNumberFieldInput />
                                <UiNumberFieldIncrement />
                            </UiNumberFieldContent>
                        </UiNumberField>
                    </UiField>

                    <UiField class="sm:col-span-2">
                        <UiFieldLabel>{{ $t('links.expiration') }}</UiFieldLabel>
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
                                    {{ $t(option.label) }}
                                </UiSelectItem>
                            </UiSelectContent>
                        </UiSelect>
                        <UiFieldDescription>
                            {{ $t('links.expirationDescription') }}
                        </UiFieldDescription>
                    </UiField>
                </UiFieldGroup>

                <UiDialogFooter>
                    <UiButton type="button" variant="outline" @click="dialog.open = false">
                        {{ $t('common.cancel') }}
                    </UiButton>
                    <UiButton type="submit" :disabled="isSaving || !form.url">
                        <UiSpinner v-if="isSaving" />
                        {{ dialog.link ? $t('links.save') : $t('links.shortenSubmit') }}
                    </UiButton>
                </UiDialogFooter>
            </form>
        </UiDialogContent>
    </UiDialog>
</template>

<script setup lang="ts">
import { ICircleAlert, ILink } from '#components';

import type { LinkData } from '~/types/links';

const route = useRoute();
const { t } = useI18n();
const date = useDate();
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
    expiration: 'labels.expiration.never',
});

const columns = computed(() => [
    { key: 'code', label: t('links.shortLink'), sortable: true, class: 'w-48' },
    { key: 'url', label: t('links.target') },
    { key: 'clicks', label: t('links.clicks'), sortable: true, descFirst: true, class: 'w-24' },
    { key: 'expiresAt', label: t('links.expires'), class: 'hidden w-32 lg:table-cell' },
    {
        key: 'createdAt',
        label: t('links.created'),
        sortable: true,
        descFirst: true,
        class: 'hidden w-32 xl:table-cell',
    },
    { key: 'actions', label: '', class: 'w-24' },
]);

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
    $toast.success(t('links.copied'));
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
            $toast.success(t('links.saved'));
        } else {
            copy(link.shortUrl);
            $toast.success(t('links.shortCopied'));
        }
    } catch (error: any) {
        formErrors.value = error.data?.data?.formErrors;
        if (!formErrors.value) $toast.error(getErrorMessage(error));
    }

    isSaving.value = false;
};

const handleDelete = async (link: LinkData) => {
    await askConfirmation({
        title: t('links.deleteTitle', { code: link.code }),
        description: t('links.deleteDescription'),
        confirmText: t('links.delete'),
        destructive: true,
        action: async () => {
            try {
                await $fetch(`/api/links/${link.id}`, { method: 'DELETE' });
                $toast.success(t('links.deleted'));
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
