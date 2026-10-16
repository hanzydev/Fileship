<template>
    <div class="flex min-h-svh flex-col">
        <Head>
            <Title>{{ $t('view.verifyPassword') }}</Title>
        </Head>

        <PasswordGate
            :title="$t('view.protectedLink')"
            :disabled="isVerifying"
            :error="passwordError"
            @submit="handlePassword"
        />
    </div>
</template>

<script setup lang="ts">
const route = useRoute();

const isVerifying = ref(false);
const passwordError = ref('');

const handlePassword = async (password: string) => {
    isVerifying.value = true;
    passwordError.value = '';

    try {
        const { url } = await $fetch('/api/links/unlock', {
            method: 'POST',
            body: { code: route.params.code, password },
        });

        await navigateTo(url, { external: true });
    } catch (error: any) {
        passwordError.value = getErrorMessage(error);
        isVerifying.value = false;
    }
};
</script>
