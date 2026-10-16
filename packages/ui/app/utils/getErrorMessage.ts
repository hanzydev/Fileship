export const getErrorMessage = (error: any) =>
    error?.data?.message || useNuxtApp().$i18n.t('common.somethingWentWrong');
