<template>
    <component
        :is="component"
        flex="~ items-center"
        rounded-md
        px4
        py2
        active:scale-95
        motion-safe:transition-all
        :class="[
            {
                // variants
                'bg-fs-accent': variant === 'accent',
                'bg-fs-overlay-2': variant === 'primary',
                'bg-fs-overlay-3 ring-1 ring-fs-overlay-4':
                    variant === 'secondary',
                'bg-transparent ring-1 ring-red-500': variant === 'danger',
                'bg-red-500': variant === 'dangerFill',
                'bg-transparent ring-1 ring-fs-accent': variant === 'outline',

                // alignments
                'justify-start text-left': alignment === 'left',
                'justify-center text-center': alignment === 'center',
                'justify-end text-right': alignment === 'right',
            },
            $attrs.disabled
                ? // disabled
                  'cursor-not-allowed op50'
                : {
                      // hover
                      'hover:(ring-1 ring-white)': variant === 'accent',
                      'hover:bg-fs-overlay-3 hover:(ring-1 ring-fs-accent)':
                          variant === 'primary',
                      'hover:(bg-fs-overlay-4 ring-fs-accent)':
                          variant === 'secondary',
                      'hover:bg-red-500': variant === 'danger',
                      'hover:(bg-red-600 ring-1 ring-white)':
                          variant === 'dangerFill',
                      'hover:bg-fs-accent': variant === 'outline',
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
import { NuxtLink } from '#components';

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
    if (href) {
        if (href.startsWith('http') || href.startsWith('/api')) {
            return 'a';
        }

        return NuxtLink;
    }

    return 'button';
});
</script>
