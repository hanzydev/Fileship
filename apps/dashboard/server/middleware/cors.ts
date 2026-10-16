export default defineEventHandler((event) => {
    if (isPreflightRequest(event)) {
        setResponseStatus(event, 204, 'No Content');
        return 'OK';
    }
});
