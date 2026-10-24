export const describeError = (error: unknown, kind: TaskKind = 'file') => {
    if (error instanceof ExtensionError) return t(error.key, error.params);
    if (!(error instanceof ApiError)) return t('common.somethingWentWrong');

    if (error.status === 0) return t(error.data?.timeout ? 'errors.timeout' : 'errors.unreachable');
    if (error.data?.quotaExceeded) return t('errors.storageFull');
    if (error.status === 401) return t('errors.invalidToken');
    if (error.status === 403)
        return t(kind === 'link' ? 'errors.cannotShorten' : 'errors.cannotUpload');

    return error.serverMessage || t('errors.requestFailed', { status: error.status });
};
