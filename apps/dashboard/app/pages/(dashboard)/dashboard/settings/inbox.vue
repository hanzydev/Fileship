<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>Inbox</UiCardTitle>
            <UiCardDescription>
                A personal drop zone where anyone with the link can send you files.
            </UiCardDescription>
            <UiCardAction>
                <UiSwitch
                    :model-value="inbox?.enabled"
                    :disabled="!inbox || isToggling"
                    aria-label="Enable inbox"
                    @update:model-value="handleToggle"
                />
            </UiCardAction>
        </UiCardHeader>
        <UiCardContent v-if="inbox" class="flex flex-col gap-6">
            <form class="flex flex-col gap-3" @submit.prevent="handleSaveSlug">
                <UiField :data-invalid="!!slugError">
                    <UiFieldLabel for="inbox-slug">Inbox link</UiFieldLabel>
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
                    variant="outline"
                    class="self-end"
                    :disabled="isSavingSlug || slug === (inbox.slug || inbox.id)"
                >
                    <UiSpinner v-if="isSavingSlug" />
                    Save link
                </UiButton>
            </form>

            <form class="flex flex-col gap-3" @submit.prevent="handleSavePassword">
                <UiField>
                    <UiFieldLabel for="inbox-password">Password</UiFieldLabel>
                    <UiInput
                        id="inbox-password"
                        v-model="password"
                        type="password"
                        placeholder="No password"
                        autocomplete="new-password"
                    />
                    <UiFieldDescription>Leave empty to let anyone send files.</UiFieldDescription>
                </UiField>
                <UiButton
                    type="submit"
                    variant="outline"
                    class="self-end"
                    :disabled="isSavingPassword || password === (inbox.password || '')"
                >
                    <UiSpinner v-if="isSavingPassword" />
                    Save password
                </UiButton>
            </form>
        </UiCardContent>
        <UiCardContent v-else>
            <UiSkeleton class="h-32 rounded-2xl" />
        </UiCardContent>
        <UiCardFooter v-if="inbox">
            <UiButton variant="ghost" as-child>
                <NuxtLink :to="`/dashboard/folders/${inbox.folderId}`">
                    <IInbox />
                    Open inbox folder
                    <IArrowRight />
                </NuxtLink>
            </UiButton>
        </UiCardFooter>
    </UiCard>
</template>

<script setup lang="ts">
const { $toast } = useNuxtApp();
const { copy } = useClipboard({ legacy: true });

const { data: inbox, refresh } = useFetch('/api/users/@me/inbox', { server: false });

const slug = ref('');
const password = ref('');
const slugError = ref<string>();
const isToggling = ref(false);
const isSavingSlug = ref(false);
const isSavingPassword = ref(false);

const inboxBaseUrl = computed(() => buildPublicUrl('/inbox/'));

const update = async (body: Record<string, unknown>) => {
    await $fetch('/api/users/@me/inbox', { method: 'PATCH', body });
    await refresh();
};

const handleToggle = async (enabled: boolean) => {
    isToggling.value = true;

    try {
        await update({ enabled });
        $toast.success(enabled ? 'Inbox enabled' : 'Inbox disabled');
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
        $toast.success('Inbox link updated');
    } catch (error: any) {
        slugError.value = getErrorMessage(error);
    }

    isSavingSlug.value = false;
};

const handleSavePassword = async () => {
    isSavingPassword.value = true;

    try {
        await update({ password: password.value || null });
        $toast.success('Inbox password updated');
    } catch (error: any) {
        $toast.error(getErrorMessage(error));
    }

    isSavingPassword.value = false;
};

const handleCopy = () => {
    copy(buildPublicUrl(`/inbox/${inbox.value!.slug || inbox.value!.id}`));
    $toast.success('Inbox link copied to clipboard');
};

watch(inbox, (value) => {
    if (!value) return;

    slug.value = value.slug || value.id;
    password.value = value.password || '';
});
</script>
