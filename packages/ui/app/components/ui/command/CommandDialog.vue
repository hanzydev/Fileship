<script setup lang="ts">
import type { DialogRootEmits, DialogRootProps } from 'reka-ui'
import { useForwardPropsEmits } from 'reka-ui'
import type { HTMLAttributes } from 'vue'

import { Dialog, DialogContent, DialogDescription, DialogHeader, DialogTitle } from '#layers/ui/app/components/ui/dialog'
import { cn } from '#layers/ui/app/utils'

import Command from './Command.vue'

const props = withDefaults(defineProps<DialogRootProps & {
  title?: string
  description?: string
  class?: HTMLAttributes['class']
  showCloseButton?: boolean
}>(), {
  showCloseButton: false,
})
const emits = defineEmits<DialogRootEmits>()

const forwarded = useForwardPropsEmits(props, emits)
</script>

<template>
  <Dialog v-slot="slotProps" v-bind="forwarded">
    <DialogContent
      :class="cn('rounded-4xl! top-1/3 translate-y-0 overflow-hidden p-0', props.class)"
      :show-close-button="showCloseButton"
    >
      <DialogHeader class="sr-only">
        <DialogTitle>{{ title ?? $t('common.commandPalette') }}</DialogTitle>
        <DialogDescription>{{ description ?? $t('common.commandPaletteDescription') }}</DialogDescription>
      </DialogHeader>
      <Command>
        <slot v-bind="slotProps" />
      </Command>
    </DialogContent>
  </Dialog>
</template>
