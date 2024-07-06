<template>
    <UiModal v-model="isOpen">
        <form p8 space-y-4 @submit.prevent="handleEdit">
            <h2>Edit Code</h2>

            <UiInput
                v-model="editData.cloned.value!.title"
                label="Title"
                type="text"
                :error="formErrors?.title?._errors?.[0]"
                :disabled="updating"
                required
                wfull
            />
            <div space-y-1>
                <UiLabel
                    :error="formErrors?.code?._errors?.[0]"
                    :disabled="updating"
                    :for="id"
                    required
                >
                    Code
                </UiLabel>
                <div
                    :id
                    :class="
                        editData.cloned.value!.language.label !==
                            'Plain Text' && 'space-y-4 rounded-md bg-fs3 p4'
                    "
                >
                    <UiTabs
                        v-if="
                            editData.cloned.value!.language.label !==
                            'Plain Text'
                        "
                        v-model="selectedTab"
                        :items="[
                            {
                                label: 'Code',
                                icon: 'heroicons-solid:code',
                            },
                            {
                                label: 'Preview',
                                icon: 'heroicons-solid:eye',
                                condition: () =>
                                    editData.cloned.value!.language.label !==
                                        'Plain Text' &&
                                    editData.cloned.value!.code.trim() !== '',
                            },
                        ]"
                        p0="!"
                    />
                    <UiTextArea
                        v-if="selectedTab === 'Code'"
                        v-model="editData.cloned.value!.code"
                        :class="
                            editData.cloned.value!.language.label ===
                                'Plain Text' && '!rounded'
                        "
                        :error="formErrors?.code?._errors?.[0]"
                        :disabled="updating"
                        required
                        wfull
                    />
                    <CodeBlock
                        v-if="
                            editData.cloned.value!.language.label !==
                            'Plain Text'
                        "
                        v-show="selectedTab === 'Preview'"
                        :language="editData.cloned.value!.language.hljs"
                        :code="editData.cloned.value!.code"
                    />
                </div>
            </div>
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
                caption="Set to 0 for unlimited views"
                type="number"
                :min="0"
                :error="formErrors?.maxViews?._errors?.[0]"
                :disabled="updating"
            />
            <ExpirationPicker
                ref="expirationPickerRef"
                v-model="editData.cloned.value!.expiration"
            >
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
            <LanguagePicker v-model="editData.cloned.value!.language">
                <UiInput
                    v-model="editData.cloned.value!.language.label"
                    label="Language"
                    type="string"
                    :error="formErrors?.language?._errors?.[0]"
                    :disabled="updating"
                    readonly
                    wfull
                    cursor-pointer="!"
                />
            </LanguagePicker>

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
import { toast } from 'vue-sonner';

import { ExpirationPicker } from '#components';

const { data } = defineProps<{
    data: CodeData;
}>();

const isOpen = defineModel<boolean>({ required: true });

const formErrors = ref();
const updating = ref(false);

const id = useId();
const selectedTab = ref('Code');

const expirationPickerRef = ref<InstanceType<typeof ExpirationPicker>>();
const expiration = computed(() =>
    data.expiresAt && expirationPickerRef.value
        ? expirationPickerRef.value.predictExpiration(data.expiresAt)
        : {
              label: 'Never',
              value: null as number | null,
          },
);

const editData = useCloned({
    ...data,
    language: languages[0]!,
    expiration: expiration.value!,
});

const handleEdit = async () => {
    updating.value = true;
    formErrors.value = {};

    try {
        await $fetch(`/api/codes/${data!.id}`, {
            method: 'PATCH',
            body: {
                title: editData.cloned.value!.title,
                code: editData.cloned.value!.code,
                password: editData.cloned.value!.password,
                maxViews: editData.cloned.value!.maxViews,
                expiration: editData.cloned.value!.expiration.value,
                language: editData.cloned.value!.language.hljs,
            },
        });

        isOpen.value = false;

        toast.success('Code updated successfully');
    } catch (error: any) {
        if (!error.data.data) toast.error(error.data.message);
        formErrors.value = error.data.data;
    }

    updating.value = false;
};
</script>
