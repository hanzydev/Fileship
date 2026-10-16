<template>
    <UiDropdownMenu>
        <UiDropdownMenuTrigger as-child>
            <UiButton variant="ghost" size="icon-sm" :aria-label="$t('layout.userMenu.actions')">
                <IEllipsis />
            </UiButton>
        </UiDropdownMenuTrigger>
        <UiDropdownMenuContent align="end" class="w-56">
            <UiDropdownMenuItem
                v-if="!user.superAdmin || currentUser!.superAdmin"
                @select="emit('edit')"
            >
                <IPencil />
                {{ $t('layout.userMenu.edit') }}
            </UiDropdownMenuItem>
            <template
                v-if="user.id !== currentUser!.id && (!user.superAdmin || currentUser!.superAdmin)"
            >
                <UiDropdownMenuItem @select="emit('impersonate')">
                    <IVenetianMask />
                    {{ $t('layout.userMenu.impersonate') }}
                </UiDropdownMenuItem>
                <UiDropdownMenuSeparator />
                <UiDropdownMenuItem variant="destructive" @select="emit('delete')">
                    <ITrash2 />
                    {{ $t('layout.userMenu.delete') }}
                </UiDropdownMenuItem>
            </template>
        </UiDropdownMenuContent>
    </UiDropdownMenu>
</template>

<script setup lang="ts">
import type { UserData } from '~/types/users';

defineProps<{
    user: UserData;
}>();

const emit = defineEmits<{
    edit: [];
    impersonate: [];
    delete: [];
}>();

const currentUser = useAuthUser();
</script>
