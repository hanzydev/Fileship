<template>
    <div>
        <Head>
            <Title v-if="!data">Verify Password</Title>
            <Title v-else>{{ data!.title }}</Title>
        </Head>

        <UiCentered v-if="!data">
            <VerifyPassword
                :disabled="passwordDisabled"
                :error="passwordError"
                @password="handlePassword"
            />
        </UiCentered>
        <CodeBlock
            v-else
            :language="data!.language"
            :code="data!.code"
            full-screen
        />
    </div>
</template>

<script setup lang="ts">
const route = useRoute();

const { data: _data, error } = await useFetch(`/api/codes/${route.params.id}`, {
    query: { log: true },
    headers: useRequestHeaders(['x-forwarded-for', 'host']),
});

const data = ref(_data.value);

if (error.value?.statusCode === 404) {
    throw createError({
        statusCode: 404,
        statusMessage: 'Not Found',
        message: 'Code not found',
    });
}

const passwordError = ref<string>();
const passwordDisabled = ref<boolean>(false);

const handlePassword = async (password: string) => {
    passwordError.value = undefined;
    passwordDisabled.value = true;

    try {
        const code = await $fetch(`/api/codes/${route.params.id}`, {
            query: {
                log: true,
                password,
            },
        });

        data.value = code;
    } catch (error: any) {
        passwordError.value = error.data.message;
        passwordDisabled.value = false;
    }
};
</script>
