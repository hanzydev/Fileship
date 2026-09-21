<script setup lang="ts">
import { cva, type VariantProps } from 'class-variance-authority';
import type { ClassValue } from 'vue';

const textBlockVariants = cva('', {
    variants: {
        variant: {
            h1: 'scroll-m-20 text-4xl font-extrabold tracking-tight lg:text-5xl',
            h2: 'scroll-m-20 border-b pb-2 text-3xl font-semibold tracking-tight first:mt-0',
            h3: 'scroll-m-20 text-2xl font-semibold tracking-tight',
            h4: 'scroll-m-20 text-xl font-semibold tracking-tight',
            p: 'leading-7 [&:not(:first-child)]:mt-6',
            blockquote: 'mt-6 border-l-2 pl-6 italic',
            table: 'w-full',
            list: 'my-6 ml-6 list-disc [&>li]:mt-2',
            inlineCode:
                'relative rounded bg-muted px-[0.3rem] py-[0.2rem] font-mono text-sm font-semibold',
            lead: 'text-xl text-muted-foreground',
            large: 'text-lg font-semibold',
            small: 'text-sm font-medium leading-none',
            muted: 'text-sm text-muted-foreground',
        },
    },
    defaultVariants: {
        variant: 'p',
    },
});

export type TextBlockVariants = VariantProps<typeof textBlockVariants>;

const defaultTags = {
    h1: 'h1',
    h2: 'h2',
    h3: 'h3',
    h4: 'h4',
    p: 'p',
    blockquote: 'blockquote',
    table: 'table',
    list: 'ul',
    inlineCode: 'code',
    lead: 'p',
    large: 'div',
    small: 'small',
    muted: 'p',
} as const;

const { variant = 'p', class: customClass } = defineProps<{
    variant?: NonNullable<TextBlockVariants['variant']>;
    class?: ClassValue;
}>();

const elementRef = useTemplateRef<HTMLElement>('elementRef');

defineExpose({ element: elementRef });

const targetTag = computed(() => defaultTags[variant]);
</script>

<template>
    <component
        :is="targetTag"
        ref="elementRef"
        :class="cn(textBlockVariants({ variant }), customClass)"
    >
        <slot />
    </component>
</template>
