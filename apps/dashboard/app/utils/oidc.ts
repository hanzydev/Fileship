const OIDC_ERRORS = [
    'unavailable',
    'expired',
    'denied',
    'failed',
    'noAccount',
    'conflict',
    'tooManyAttempts',
    'alreadyLinked',
    'wrongAccount',
    'wrongAddress',
];

export const oidcErrorKey = (status: string) =>
    `auth.oidc.errors.${OIDC_ERRORS.includes(status) ? status : 'failed'}`;
