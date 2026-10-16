<template>
    <div>
        <Head>
            <Title>{{ $t('connect.title') }}</Title>
        </Head>
        <Centered>
            <UiCard class="w-full max-w-sm">
                <UiCardHeader>
                    <UiCardTitle>{{ $t('connect.heading') }}</UiCardTitle>
                    <UiCardDescription>
                        {{ $t('connect.description', { version: MIN_FILESHIP_VERSION }) }}
                    </UiCardDescription>
                </UiCardHeader>
                <UiCardContent>
                    <form @submit.prevent="handleSubmit">
                        <UiFieldGroup>
                            <UiField :data-invalid="!!error">
                                <UiFieldLabel for="instance-url">
                                    {{ $t('connect.serverAddress') }}
                                </UiFieldLabel>
                                <UiInput
                                    id="instance-url"
                                    v-model="url"
                                    :placeholder="$t('connect.addressPlaceholder')"
                                    autocomplete="url"
                                    autofocus
                                    :disabled="isBusy"
                                />
                                <UiFieldError v-if="error" :errors="[error]" />
                            </UiField>
                            <UiField>
                                <UiButton type="submit" :disabled="isBusy || !url.trim()">
                                    <UiSpinner v-if="isBusy" />
                                    {{ isBusy ? status : $t('common.continue') }}
                                </UiButton>
                                <UiFieldDescription v-if="isLoggingIn" class="text-center">
                                    {{ $t('connect.loginHint') }}
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
const { t } = useI18n();

const url = ref('');
const error = ref('');
const status = ref('');
const isBusy = ref(false);
const isLoggingIn = ref(false);

const handleSubmit = async () => {
    error.value = '';
    isBusy.value = true;
    status.value = t('connect.checking');

    try {
        const instance = await desktop('instance:check', url.value);

        url.value = instance.url;
        status.value = t('connect.loggingIn', { siteName: instance.siteName });
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
