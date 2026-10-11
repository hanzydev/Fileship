<template>
    <UiInputGroup>
        <UiInputGroupInput
            v-model="model"
            v-bind="$attrs"
            :type="isVisible ? 'text' : 'password'"
        />
        <UiInputGroupAddon align="inline-end">
            <UiInputGroupButton
                size="icon-xs"
                :aria-label="isVisible ? 'Hide password' : 'Show password'"
                :aria-pressed="isVisible"
                @click="isVisible = !isVisible"
            >
                <IEyeOff v-if="isVisible" />
                <IEye v-else />
            </UiInputGroupButton>
        </UiInputGroupAddon>
    </UiInputGroup>
</template>

<script setup lang="ts">
defineOptions({ inheritAttrs: false });

const model = defineModel<string>();

const isVisible = ref(false);
</script>
