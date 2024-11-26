<template>
    <UiModal
        v-model="isOpen"
        @closed="
            url.destinationUrl = '';
            url.vanity = '';
            url.password = '';
            url.maxViews = 0;
            formErrors = {};
        "
    >
        <form p8 space-y-4 @submit.prevent="handleSubmit">
            <h2>Shorten URL</h2>

            <UiInput
                v-model="url.vanity"
                wfull
                label="Vanity"
                type="text"
                :error="formErrors?.vanity?._errors?.[0]"
                :disabled
            />
            <UiInput
                v-model="url.destinationUrl"
                label="Destination Url"
                type="text"
                :error="formErrors?.destinationUrl?._errors?.[0]"
                required
                wfull
                :disabled
            />
            <UiInput
                v-model="url.password"
                wfull
                label="Password"
                type="password"
                :error="formErrors?.password?._errors?.[0]"
                :disabled
            />
            <UiInput
                v-model="url.maxViews"
                wfull
                label="Max Views"
                caption="Set to 0 for unlimited views."
                type="number"
                :min="0"
                :error="formErrors?.maxViews?._errors?.[0]"
                :disabled
            />
            <ExpirationPicker v-model="url.expiration">
                <UiInput
                    v-model="url.expiration.label"
                    label="Expiration"
                    type="string"
                    :error="formErrors?.expiresAt?._errors?.[0]"
                    :disabled
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
                    @click="(isOpen = false)"
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
                    :loading="disabled"
                    :disabled
                >
                    Save
                </UiButton>
            </div>
        </form>
    </UiModal>
</template>

<script setup lang="ts">
import { toast } from 'vue-sonner';

const isOpen = defineModel<boolean>({ required: true });

const formErrors = ref();
const disabled = ref(false);

const url = reactive({
    destinationUrl: '',
    vanity: '',
    password: '',
    maxViews: 0,
    expiration: {
        label: 'Never',
        value: null as number | null,
    },
});

const handleSubmit = async () => {
    disabled.value = true;
    formErrors.value = {};

    try {
        await $fetch('/api/urls', {
            method: 'POST',
            body: {
                ...url,
                expiration: url.expiration.value,
            },
        });

        isOpen.value = false;

        toast.success('URL shortened successfully');
    } catch (error: any) {
        formErrors.value = error.data.data?.formErrors;
    }

    disabled.value = false;
};
</script>
