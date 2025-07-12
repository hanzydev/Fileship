export const unauthorizedError = createError({
    statusCode: 401,
    statusMessage: 'Unauthorized',
    message: 'You must be logged in to perform this action',
});

export const forbiddenError = createError({
    statusCode: 403,
    statusMessage: 'Forbidden',
    message: 'You do not have permission to perform this action',
});
