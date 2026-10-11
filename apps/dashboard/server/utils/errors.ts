export const unauthorizedError = createError({
    status: 401,
    message: 'You must be logged in to perform this action',
});

export const forbiddenError = createError({
    status: 403,
    message: 'You do not have permission to perform this action',
});
