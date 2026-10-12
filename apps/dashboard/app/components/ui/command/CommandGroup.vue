<script setup lang="ts">
import type { ListboxGroupProps } from 'reka-ui'
import { ListboxGroup, ListboxGroupLabel, useId } from 'reka-ui'
import type { HTMLAttributes } from 'vue'
import { computed, onMounted, onUnmounted } from 'vue'

import { reactiveOmit } from '@vueuse/core'

import { provideCommandGroupContext, useCommand } from '.'

import { cn } from '@/utils'

const props = defineProps<ListboxGroupProps & {
  class?: HTMLAttributes['class']
  heading?: string
}>()

const delegatedProps = reactiveOmit(props, 'class')

const { allGroups, filterState } = useCommand()
const id = useId()

const isRender = computed(() => {
  if (!filterState.search || filterState.filtered.groups.has(id))
    return true

  return [...(allGroups.value.get(id) ?? [])].some(itemId => filterState.filtered.items.get(itemId) !== 0)
})

provideCommandGroupContext({ id })
onMounted(() => {
  if (!allGroups.value.has(id))
    allGroups.value.set(id, new Set())
})
onUnmounted(() => {
  allGroups.value.delete(id)
})
</script>

<template>
  <ListboxGroup
    v-bind="delegatedProps"
    :id="id"
    data-slot="command-group"
    :class="cn('text-foreground **:[[cmdk-group-heading]]:text-muted-foreground overflow-hidden p-1.5 **:[[cmdk-group-heading]]:px-3 **:[[cmdk-group-heading]]:py-2 **:[[cmdk-group-heading]]:text-xs **:[[cmdk-group-heading]]:font-medium', props.class)"
    :hidden="isRender ? undefined : true"
  >
    <ListboxGroupLabel v-if="heading" data-slot="command-group-heading" cmdk-group-heading class="">
      {{ heading }}
    </ListboxGroupLabel>
    <slot />
  </ListboxGroup>
</template>
