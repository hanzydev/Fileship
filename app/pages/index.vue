<template>
    <div></div>
</template>

<script setup lang="ts">
const currentUser = useAuthUser();
await navigateTo(currentUser.value ? '/dashboard' : '/login');
</script>
