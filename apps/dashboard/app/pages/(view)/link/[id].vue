<template>
    <div>
        <Head>
            <Title>Verify Password</Title>
        </Head>

        <UiCentered>
            <VerifyPassword
                :disabled="passwordDisabled"
                :error="passwordError"
                @password="handlePassword"
            />
        </UiCentered>
    </div>
</template>

<script setup lang="ts">
const route = useRoute();

const { data, error } = await useFetch<UrlData>(`/api/urls/${route.params.id}`, {
    query: { log: true },
    headers: useRequestHeaders(['x-forwarded-for', 'host']),
});

if (data.value) {
    await navigateTo(data.value.destinationUrl, { external: true });
} else if (error.value?.statusCode === 404) {
    throw createError({
        statusCode: 404,
        message: 'URL not found',
    });
}

const passwordError = ref<string>();
const passwordDisabled = ref<boolean>(false);

const handlePassword = async (password: string) => {
    passwordError.value = undefined;
    passwordDisabled.value = true;

    try {
        const url = await $fetch<UrlData>(`/api/urls/${route.params.id}`, {
            query: {
                log: true,
                password,
            },
        });

        await navigateTo(url.destinationUrl, { external: true });
    } catch (error: any) {
        passwordError.value = error.data.message;
        passwordDisabled.value = false;
    }
};
</script>
