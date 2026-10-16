<template>
    <UiSonner
        position="top-center"
        :theme="colorMode.value as never"
        :toast-options="{
            classes: {
                toast: 'rounded-3xl',
                actionButton: cn(buttonVariants({ size: 'xs' })),
                cancelButton: cn(buttonVariants({ variant: 'outline', size: 'xs' })),
            },
        }"
    />
</template>

<script setup lang="ts">
import { buttonVariants } from '#layers/ui/app/components/ui/button';

const colorMode = useColorMode();
</script>
