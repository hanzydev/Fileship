export const getErrorMessage = (error: any) =>
    error?.data?.message || error?.statusText || 'Something went wrong';
