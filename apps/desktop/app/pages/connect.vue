<template>
    <div>
        <Head>
            <Title>Connect</Title>
        </Head>
        <Centered>
            <UiCard class="w-full max-w-sm">
                <UiCardHeader>
                    <UiCardTitle>Connect to Fileship</UiCardTitle>
                    <UiCardDescription>
                        Enter the address of your Fileship server. It needs Fileship
                        {{ MIN_FILESHIP_VERSION }} or later.
                    </UiCardDescription>
                </UiCardHeader>
                <UiCardContent>
                    <form @submit.prevent="handleSubmit">
                        <UiFieldGroup>
                            <UiField :data-invalid="!!error">
                                <UiFieldLabel for="instance-url">Server address</UiFieldLabel>
                                <UiInput
                                    id="instance-url"
                                    v-model="url"
                                    placeholder="files.example.com"
                                    autocomplete="url"
                                    autofocus
                                    :disabled="isBusy"
                                />
                                <UiFieldError v-if="error" :errors="[error]" />
                            </UiField>
                            <UiField>
                                <UiButton type="submit" :disabled="isBusy || !url.trim()">
                                    <UiSpinner v-if="isBusy" />
                                    {{ isBusy ? status : 'Continue' }}
                                </UiButton>
                                <UiFieldDescription v-if="isLoggingIn" class="text-center">
                                    Log in in the window that opened. Close it to cancel.
                                </UiFieldDescription>
                            </UiField>
                        </UiFieldGroup>
                    </form>
                </UiCardContent>
            </UiCard>
        </Centered>
    </div>
</template>

<script setup lang="ts">
import { MIN_FILESHIP_VERSION } from '../../shared/ipc';

const account = useAccount();

const url = ref('');
const error = ref('');
const status = ref('');
const isBusy = ref(false);
const isLoggingIn = ref(false);

const handleSubmit = async () => {
    error.value = '';
    isBusy.value = true;
    status.value = 'Checking server...';

    try {
        const instance = await desktop('instance:check', url.value);

        url.value = instance.url;
        status.value = `Logging in to ${instance.siteName}...`;
        isLoggingIn.value = true;

        account.value = await desktop('account:login', instance.url);
        await navigateTo('/home');
    } catch (cause: any) {
        error.value = cause.message;
    }

    isBusy.value = false;
    isLoggingIn.value = false;
};
</script>
