<template>
    <UiDropdownMenu>
        <UiDropdownMenuTrigger as-child>
            <UiButton variant="ghost" size="icon-sm">
                <IEllipsis />
            </UiButton>
        </UiDropdownMenuTrigger>
        <UiDropdownMenuContent align="end" class="w-56">
            <UiDropdownMenuItem
                v-if="!user.superAdmin || currentUser!.superAdmin"
                @select="emit('edit')"
            >
                <IPencil />
                Edit
            </UiDropdownMenuItem>
            <template
                v-if="user.id !== currentUser!.id && (!user.superAdmin || currentUser!.superAdmin)"
            >
                <UiDropdownMenuItem @select="emit('impersonate')">
                    <IVenetianMask />
                    Impersonate
                </UiDropdownMenuItem>
                <UiDropdownMenuSeparator />
                <UiDropdownMenuItem variant="destructive" @select="emit('delete')">
                    <ITrash2 />
                    Delete
                </UiDropdownMenuItem>
            </template>
        </UiDropdownMenuContent>
    </UiDropdownMenu>
</template>

<script setup lang="ts">
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
