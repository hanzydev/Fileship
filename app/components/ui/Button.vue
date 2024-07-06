<template>
    <component
        :is="component"
        flex="~ items-center"
        rounded-md
        px4
        py2
        active:scale-95
        hover:ring-1
        motion-safe:transition-all
        :class="[
            {
                'bg-fs-accent hover:ring-white': variant === 'accent',
                'bg-fs3 hover:bg-fs2 hover:ring-fs-accent':
                    variant === 'primary',
                'bg-fs2 hover:bg-fs1 hover:ring-fs-accent':
                    variant === 'secondary',
                'bg-transparent ring-2 ring-red-500 hover:bg-red-500':
                    variant === 'danger',
                'bg-red-500 hover:bg-red-600 hover:ring-white':
                    variant === 'dangerFill',
                'bg-transparent ring-fs-accent hover:bg-fs-accent':
                    variant === 'outline',
                'cursor-not-allowed op50': $attrs.disabled,
                'justify-start text-left': alignment === 'left',
                'justify-center text-center': alignment === 'center',
                'justify-end text-right': alignment === 'right',
            },
        ]"
        :target="
            href ? (href?.startsWith('http') ? '_blank' : '_self') : undefined
        "
        :href
        type="button"
    >
        <Icon
            v-if="icon && !loading"
            :name="icon"
            :size="iconSize"
            flex-shrink-0
            :class="iconClass"
        />
        <UiSpinner v-else-if="loading" :size="+iconSize" />
        <slot />
    </component>
</template>

<script setup lang="ts">
const {
    variant = 'primary',
    alignment = 'left',
    iconSize = '16',
    href,
} = defineProps<{
    variant?:
        | 'accent'
        | 'primary'
        | 'secondary'
        | 'danger'
        | 'dangerFill'
        | 'outline';
    alignment?: 'left' | 'center' | 'right';
    loading?: boolean;
    icon?: string;
    iconSize?: string;
    iconClass?: unknown;
    href?: string;
}>();

const component = computed(() => {
    if (href) return resolveComponent('NuxtLink');
    return 'button';
});
</script>
