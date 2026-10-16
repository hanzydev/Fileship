export default defineNitroPlugin(() => {
    runTask('db:prune');
});
