<template>
    <UiPopover v-model:open="isOpen">
        <UiPopoverTrigger as-child>
            <UiButton :id="id" variant="outline" class="w-full justify-between">
                <span class="flex min-w-0 items-center gap-2">
                    <slot name="icon">
                        <IUser />
                    </slot>
                    <span class="truncate">{{ label }}</span>
                </span>
                <IChevronsUpDown class="opacity-50" />
            </UiButton>
        </UiPopoverTrigger>
        <UiPopoverContent class="w-(--reka-popover-trigger-width) min-w-56 p-0" align="end">
            <UiCommand>
                <UiCommandInput
                    :placeholder="$t('instance.import.searchUsers')"
                    @update:model-value="query = $event"
                />
                <UiCommandList class="mb-1">
                    <UiCommandGroup v-if="$slots.default" class="px-1 pb-0">
                        <slot :close="close" />
                    </UiCommandGroup>
                    <UiCommandGroup class="px-1 pb-0">
                        <VirtualList
                            :items="users"
                            :item-key="(user) => user.id"
                            :estimate-size="36"
                            :gap="0"
                            contained
                            @reach-end="loadMore"
                        >
                            <template #default="{ item }">
                                <UiCommandItem
                                    :value="item.id"
                                    :data-checked="selected === item.id"
                                    @select="handleSelect(item)"
                                >
                                    <IUser />
                                    <span class="truncate">{{ item.username }}</span>
                                </UiCommandItem>
                            </template>
                        </VirtualList>
                    </UiCommandGroup>
                </UiCommandList>
            </UiCommand>
        </UiPopoverContent>
    </UiPopover>
</template>

<script setup lang="ts">
import type { UserData } from '~/types/users';

defineProps<{
    id?: string;
    label: string;
    selected?: string | null;
}>();

const emit = defineEmits<{
    select: [user: { id: string; username: string }];
}>();

const isOpen = ref(false);
const query = ref('');

const { items: users, loadMore } = usePaginatedList<UserData>(
    'users',
    '/api/users',
    () => ({ q: query.value.trim() }),
    { limit: 30, enabled: isOpen },
);

const close = () => (isOpen.value = false);

const handleSelect = (user: UserData) => {
    emit('select', { id: user.id, username: user.username });
    close();
};

watch(isOpen, (value) => value && (query.value = ''));
</script>
