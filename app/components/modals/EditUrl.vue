<template>
    <UiModal v-model="isOpen">
        <form p8 space-y-4 @submit.prevent="handleEdit">
            <h2>Edit URL</h2>

            <UiInput
                v-model="editData.cloned.value!.vanity"
                required
                wfull
                label="Vanity"
                type="text"
                :error="formErrors?.vanity?._errors?.[0]"
                :disabled="updating"
            />
            <UiInput
                v-model="editData.cloned.value!.destinationUrl"
                label="Destination Url"
                type="text"
                :error="formErrors?.destinationUrl?._errors?.[0]"
                required
                wfull
                :disabled="updating"
            />
            <UiInput
                v-model="editData.cloned.value!.password!"
                wfull
                label="Password"
                type="password"
                :error="formErrors?.password?._errors?.[0]"
                :disabled="updating"
            />
            <UiInput
                v-model="editData.cloned.value!.maxViews"
                wfull
                label="Max Views"
                caption="Set to 0 for unlimited views."
                type="number"
                :min="0"
                :error="formErrors?.maxViews?._errors?.[0]"
                :disabled="updating"
            />
            <ExpirationPicker ref="expirationPickerRef" v-model="editData.cloned.value!.expiration">
                <UiInput
                    v-model="editData.cloned.value!.expiration.label"
                    label="Expiration"
                    type="string"
                    :error="formErrors?.expiresAt?._errors?.[0]"
                    :disabled="updating"
                    readonly
                    wfull
                    cursor-pointer="!"
                />
            </ExpirationPicker>

            <div grid="~ cols-2 gap4">
                <UiButton
                    alignment="center"
                    icon="heroicons-solid:x"
                    icon-size="24"
                    wfull
                    gap2
                    @click="isOpen = false"
                >
                    Cancel
                </UiButton>
                <UiButton
                    wfull
                    gap2
                    alignment="center"
                    variant="accent"
                    type="submit"
                    icon="heroicons:pencil-16-solid"
                    icon-size="20"
                    :loading="updating"
                    :disabled="updating"
                >
                    Save
                </UiButton>
            </div>
        </form>
    </UiModal>
</template>

<script setup lang="ts">
import { ExpirationPicker } from '#components';

const { data } = defineProps<{
    data: UrlData;
}>();

const isOpen = defineModel<boolean>({ required: true });

const { $toast } = useNuxtApp();

const formErrors = ref();
const updating = ref(false);
const expirationPickerRef = ref<InstanceType<typeof ExpirationPicker>>();

const editData = useCloned({
    ...data,
    expiration: {
        label: 'Never',
        value: null as number | null,
    },
});

const handleEdit = async () => {
    updating.value = true;
    formErrors.value = {};

    try {
        await $fetch(`/api/urls/${data!.id}`, {
            method: 'PATCH',
            body: {
                vanity: editData.cloned.value!.vanity,
                destinationUrl: editData.cloned.value!.destinationUrl,
                password: editData.cloned.value!.password,
                maxViews: editData.cloned.value!.maxViews,
                expiration: editData.cloned.value!.expiration.value,
            },
        });

        isOpen.value = false;

        $toast.success('URL updated successfully');
    } catch (error: any) {
        if (!error.data.data?.formErrors) $toast.error(error.data.message);
        formErrors.value = error.data.data?.formErrors;
    }

    updating.value = false;
};

watch(isOpen, async (value) => {
    await nextTick();

    if (value && data.expiresAt && expirationPickerRef.value) {
        editData.cloned.value.expiration = expirationPickerRef.value.predictExpiration(
            data.expiresAt,
        );
    }
});

watch(
    () => data,
    (value) =>
        (editData.cloned.value = JSON.parse(
            JSON.stringify({
                ...value,
                expiration: {
                    label: 'Never',
                    value: null as number | null,
                },
            }),
        )),
);
</script>
