export default defineTask({
    meta: {
        name: 'email:storageWarnings',
        description: 'Emails users whose storage is almost full and clears the warning below 90%',
    },
    async run() {
        return { result: await checkStorageWarnings() };
    },
});
