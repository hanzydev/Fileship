import { canShortenUrls } from '~~/utils/permissions';

export default defineNuxtRouteMiddleware(() => {
    const user = useAuthUser();
    if (!user.value) return navigateTo('/login');

    if (!canShortenUrls(user)) {
        return abortNavigation({
            statusCode: 403,
            statusMessage: 'Forbidden',
            message: 'You do not have permission to access this page',
        });
    }
});