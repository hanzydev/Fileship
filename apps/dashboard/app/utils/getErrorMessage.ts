export const getErrorMessage = (error: any) =>
    error?.data?.message || error?.statusMessage || 'Something went wrong';
