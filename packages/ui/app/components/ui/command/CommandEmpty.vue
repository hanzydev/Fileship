<script setup lang="ts">
import type { PrimitiveProps } from 'reka-ui'
import { Primitive } from 'reka-ui'
import type { HTMLAttributes } from 'vue'
import { computed } from 'vue'

import { reactiveOmit } from '@vueuse/core'

import { cn } from '#layers/ui/app/utils'

import { useCommand } from '.'

const props = defineProps<PrimitiveProps & { class?: HTMLAttributes['class'] }>()

const delegatedProps = reactiveOmit(props, 'class')

const { allItems, filterState } = useCommand()
const isRender = computed(() => !!filterState.search && ![...allItems.value.keys()].some(id => filterState.filtered.items.get(id) !== 0),
)
</script>

<template>
  <Primitive
    v-if="isRender"
    data-slot="command-empty"
    v-bind="delegatedProps"
    :class="cn('py-6 text-center text-sm', props.class)"
  >
    <slot />
  </Primitive>
</template>
