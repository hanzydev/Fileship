<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('inboxSettings.title') }}</UiCardTitle>
            <UiCardDescription>
                {{ $t('inboxSettings.description') }}
            </UiCardDescription>
            <UiCardAction>
                <UiSwitch
                    :model-value="inbox?.enabled"
                    :disabled="!inbox || isToggling"
                    :aria-label="$t('inboxSettings.enable')"
                    @update:model-value="handleToggle"
                />
            </UiCardAction>
        </UiCardHeader>
        <UiCardContent v-if="inbox" class="flex flex-col gap-6">
            <form class="flex flex-col gap-3" @submit.prevent="handleSaveSlug">
                <UiField :data-invalid="!!slugError">
                    <UiFieldLabel for="inbox-slug">{{ $t('inboxSettings.link') }}</UiFieldLabel>
                    <UiInputGroup>
                        <UiInputGroupAddon class="hidden sm:flex">
                            {{ inboxBaseUrl }}
                        </UiInputGroupAddon>
                        <UiInputGroupInput id="inbox-slug" v-model="slug" />
                        <UiInputGroupAddon align="inline-end">
                            <UiInputGroupButton size="icon-xs" type="button" @click="handleCopy">
                                <ICopy />
                            </UiInputGroupButton>
                        </UiInputGroupAddon>
                    </UiInputGroup>
                    <UiFieldError v-if="slugError" :errors="[slugError]" />
                </UiField>
                <UiButton
                    type="submit"
                    class="self-end"
                    :disabled="isSavingSlug || slug === (inbox.slug || inbox.id)"
                >
                    <UiSpinner v-if="isSavingSlug" />
                    {{ $t('inboxSettings.saveLink') }}
                </UiButton>
            </form>

            <form class="flex flex-col gap-3" @submit.prevent="handleSavePassword">
                <UiField>
                    <UiFieldLabel for="inbox-password">
                        {{ $t('inboxSettings.password') }}
                    </UiFieldLabel>
                    <PasswordInput
                        id="inbox-password"
                        v-model="password"
                        :placeholder="$t('inboxSettings.noPassword')"
                        autocomplete="new-password"
                    />
                    <UiFieldDescription>
                        {{ $t('inboxSettings.passwordDescription') }}
                    </UiFieldDescription>
                </UiField>
                <UiButton
                    type="submit"
                    class="self-end"
                    :disabled="isSavingPassword || password === (inbox.password || '')"
                >
                    <UiSpinner v-if="isSavingPassword" />
                    {{ $t('inboxSettings.savePassword') }}
                </UiButton>
            </form>
        </UiCardContent>
        <UiCardContent v-else class="flex flex-col gap-6">
            <div v-for="i in 2" :key="i" class="flex flex-col gap-3">
                <UiSkeleton class="h-4 w-24 rounded-full" />
                <UiSkeleton class="h-9 w-full rounded-full" />
                <UiSkeleton class="ml-auto h-9 w-28 rounded-full" />
            </div>
        </UiCardContent>
        <UiCardFooter v-if="inbox">
            <UiButton variant="ghost" as-child>
                <NuxtLink :to="`/dashboard/folders/${inbox.folderId}`">
                    <IInbox />
                    {{ $t('inboxSettings.openFolder') }}
                    <IArrowRight />
                </NuxtLink>
            </UiButton>
        </UiCardFooter>
    </UiCard>
</template>

<script setup lang="ts">
const { $toast } = useNuxtApp();
const { t } = useI18n();
const { copy } = useClipboard({ legacy: true });

const { data: inbox, refresh } = useFetch('/api/users/@me/inbox', { server: false });

const slug = ref('');
const password = ref('');
const slugError = ref<string>();
const isToggling = ref(false);
const isSavingSlug = ref(false);
const isSavingPassword = ref(false);

const inboxBaseUrl = computed(() => getPublicUrl('/inbox/'));

const update = async (body: Record<string, unknown>) => {
    await $fetch('/api/users/@me/inbox', { method: 'PATCH', body });
    await refresh();
};

const handleToggle = async (enabled: boolean) => {
    isToggling.value = true;

    try {
        await update({ enabled });
        $toast.success(enabled ? t('inboxSettings.enabled') : t('inboxSettings.disabled'));
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    isToggling.value = false;
};

const handleSaveSlug = async () => {
    isSavingSlug.value = true;
    slugError.value = undefined;

    try {
        await update({ slug: slug.value || null });
        $toast.success(t('inboxSettings.linkUpdated'));
    } catch (error: any) {
        slugError.value = getErrorMessage(error);
    }

    isSavingSlug.value = false;
};

const handleSavePassword = async () => {
    isSavingPassword.value = true;

    try {
        await update({ password: password.value || null });
        $toast.success(t('inboxSettings.passwordUpdated'));
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    isSavingPassword.value = false;
};

const handleCopy = () => {
    copy(getPublicUrl(`/inbox/${inbox.value!.slug || inbox.value!.id}`));
    $toast.success(t('inboxSettings.linkCopied'));
};

watch(inbox, (value) => {
    if (!value) return;

    slug.value = value.slug || value.id;
    password.value = value.password || '';
});

definePageMeta({
    middleware: 'file-uploader-only',
});
</script>
