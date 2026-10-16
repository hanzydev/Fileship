<template>
    <UiInputOTP v-model="code" class="w-full" :maxlength="6">
        <UiInputOTPGroup class="flex-1">
            <UiInputOTPSlot v-for="i in 3" :key="i" :index="i - 1" class="w-full flex-1" />
        </UiInputOTPGroup>
        <UiInputOTPSeparator />
        <UiInputOTPGroup class="flex-1">
            <UiInputOTPSlot v-for="i in 3" :key="i" :index="i + 2" class="w-full flex-1" />
        </UiInputOTPGroup>
    </UiInputOTP>
</template>

<script setup lang="ts">
const code = defineModel<string>({ required: true });

const emit = defineEmits<{
    complete: [];
}>();

watch(code, (value, oldValue) => {
    if (value.length === 6 && oldValue.length < 6) emit('complete');
});
</script>
