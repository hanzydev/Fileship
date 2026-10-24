export default defineTask({
    meta: {
        name: 'files:hash',
        description: 'Hashes files that have the same size as another file of their owner',
    },
    async run() {
        await syncContentHashes();

        return { result: 'success' };
    },
});
