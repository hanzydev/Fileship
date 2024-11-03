<template>
    <UiModal
        v-model="isOpen"
        @closed="
            code.title = '';
            code.code = '';
            code.password = '';
            code.language = languages[0]!;
            code.maxViews = 0;
            code.expiration = {
                label: 'Never',
                value: null,
            };
            selectedTab = 'Code';
            formErrors = {};
        "
    >
        <form p8 space-y-4 @submit.prevent="handleSubmit">
            <h2>Share Code</h2>

            <UiInput
                v-model="code.title"
                label="Title"
                type="text"
                :error="formErrors?.title?._errors?.[0]"
                :disabled
                required
                wfull
            />
            <div space-y-1>
                <UiLabel :error="formErrors?.code?._errors?.[0]" :disabled :for="id" required>
                    Code
                </UiLabel>
                <div
                    :id
                    :class="
                        code.language.label !== 'Plain Text' &&
                        'space-y-4 rounded-md bg-fs-overlay-2 p4'
                    "
                >
                    <UiTabs
                        v-if="code.language.label !== 'Plain Text'"
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
                                    code.language.label !== 'Plain Text' && code.code.trim() !== '',
                            },
                        ]"
                        p0="!"
                    />
                    <UiTextArea
                        v-if="selectedTab === 'Code'"
                        v-model="code.code"
                        :class="code.language.label === 'Plain Text' && '!rounded'"
                        :error="formErrors?.code?._errors?.[0]"
                        :disabled
                        required
                        wfull
                    />
                    <CodeBlock
                        v-if="code.language.label !== 'Plain Text'"
                        v-show="selectedTab === 'Preview'"
                        :language="code.language.hljs"
                        :code="code.code"
                    />
                </div>
            </div>
            <UiInput
                v-model="code.password"
                wfull
                label="Password"
                type="password"
                :error="formErrors?.password?._errors?.[0]"
                :disabled
            />
            <UiInput
                v-model="code.maxViews"
                wfull
                label="Max Views"
                caption="Set to 0 for unlimited views"
                type="number"
                :min="0"
                :error="formErrors?.maxViews?._errors?.[0]"
                :disabled
            />
            <ExpirationPicker v-model="code.expiration">
                <UiInput
                    v-model="code.expiration.label"
                    label="Expiration"
                    type="string"
                    :error="formErrors?.expiresAt?._errors?.[0]"
                    :disabled
                    readonly
                    wfull
                    cursor-pointer="!"
                />
            </ExpirationPicker>
            <LanguagePicker v-model="code.language">
                <UiInput
                    v-model="code.language.label"
                    label="Language"
                    type="string"
                    :error="formErrors?.language?._errors?.[0]"
                    :disabled
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

const code = reactive({
    title: '',
    code: '',
    password: '',
    language: languages[0]!,
    maxViews: 0,
    expiration: {
        label: 'Never',
        value: null as number | null,
    },
});

const id = useId();

const selectedTab = ref('Code');

const handleSubmit = async () => {
    disabled.value = true;
    formErrors.value = {};

    try {
        await $fetch('/api/codes', {
            method: 'POST',
            body: {
                ...code,
                expiration: code.expiration.value,
                language: code.language.hljs,
            },
        });

        isOpen.value = false;

        toast.success('Code shared successfully');
    } catch (error: any) {
        formErrors.value = error.data.data?.formErrors;
    }

    disabled.value = false;
};

watch(
    () => code.language,
    (language) => {
        if (language.label === 'Plain Text' && selectedTab.value === 'Preview') {
            selectedTab.value = 'Code';
        }
    },
);
</script>
