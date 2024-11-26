<template>
    <UiModal v-model="isOpen">
        <div flex="~ col gap4 justify-center items-center" wfull p8 text-center>
            <Icon name="heroicons-solid:exclamation" size="96" />
            <h3>{{ title }}</h3>
            <p text-slate300 font-medium>
                {{ description }}
            </p>
            <div grid="~ gap2 sm:cols-2 sm:gap4" wfull>
                <UiButton
                    alignment="center"
                    variant="accent"
                    icon="heroicons-solid:x"
                    icon-size="24"
                    gap2
                    @click="(isOpen = false)"
                >
                    Cancel
                </UiButton>
                <UiButton
                    alignment="center"
                    icon="heroicons-solid:check"
                    icon-size="24"
                    gap2
                    @click="
                        $emit('confirm');
                        isOpen = false;
                    "
                >
                    Confirm
                </UiButton>
            </div>
        </div>
    </UiModal>
</template>

<script setup lang="ts">
defineProps<{
    title: string;
    description: string;
}>();

defineEmits<{
    confirm: [];
}>();

const isOpen = defineModel<boolean>({ required: false, default: true });
</script>
