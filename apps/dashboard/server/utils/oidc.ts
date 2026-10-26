import { randomBytes } from 'node:crypto';

import consola from 'consola';
import type { H3Event } from 'h3';
import * as client from 'openid-client';

type OidcIntent = 'login' | 'link' | 'verify';

type OidcFlow = {
    intent: OidcIntent;
    state: string;
    nonce: string;
    codeVerifier: string;
    userId?: string;
    sessionId?: string;
    redirectTo?: string;
    expiresAt: number;
};

export type OidcClaims = {
    issuer: string;
    subject: string;
    email: string | null;
    emailVerified: boolean;
    username: string | null;
};

const FLOW_COOKIE = 'oidcFlow';
const FLOW_PATH = '/api/auth/oidc';

export const isOidcConfigured = (settings: InstanceSettings) =>
    settings.oidcEnabled &&
    !!settings.siteUrl &&
    !!settings.oidcIssuerUrl &&
    !!settings.oidcClientId;

export const isPasswordLoginEnabled = (settings: InstanceSettings) =>
    settings.passwordLoginEnabled || !isOidcConfigured(settings);

export const canLogInWithPassword = (user: { superAdmin: boolean }, settings: InstanceSettings) =>
    user.superAdmin || isPasswordLoginEnabled(settings);

export const isSameIssuer = (a: string, b: string) => new URL(a).href === new URL(b).href;

export const isSiteHost = (event: H3Event, settings: InstanceSettings) =>
    getRequestHost(event) === new URL(settings.siteUrl).host;

export const oidcRedirectUri = (settings: InstanceSettings) =>
    new URL('/api/auth/oidc/callback', settings.siteUrl).href;

export const findOidcIdentity = async (userId: string, settings: InstanceSettings) =>
    (await prisma.oidcIdentity.findMany({ where: { userId } })).find((identity) =>
        isSameIssuer(identity.issuer, settings.oidcIssuerUrl),
    ) ?? null;

export const discoverOidc = async (issuerUrl: string, clientId: string, clientSecret: string) => {
    const issuer = new URL(issuerUrl);
    const execute = issuer.protocol === 'http:' ? [client.allowInsecureRequests] : [];
    const discovered = await client.discovery(issuer, clientId, undefined, client.None(), {
        execute,
        timeout: 10,
    });

    if (!clientSecret) return discovered;

    const methods = discovered.serverMetadata().token_endpoint_auth_methods_supported;
    const config = new client.Configuration(
        discovered.serverMetadata(),
        clientId,
        clientSecret,
        !methods || methods.includes('client_secret_basic')
            ? client.ClientSecretBasic(clientSecret)
            : client.ClientSecretPost(clientSecret),
    );

    for (const method of execute) method(config);
    config.timeout = 10;

    return config;
};

let cached: { key: string; config: Promise<client.Configuration>; expiresAt: number } | null = null;

const getOidcConfig = (settings: InstanceSettings) => {
    const key = JSON.stringify([
        settings.oidcIssuerUrl,
        settings.oidcClientId,
        settings.oidcClientSecret,
    ]);

    if (cached?.key === key && cached.expiresAt > Date.now()) return cached.config;

    const current = {
        key,
        config: discoverOidc(
            settings.oidcIssuerUrl,
            settings.oidcClientId,
            settings.oidcClientSecret,
        ),
        expiresAt: Date.now() + 60 * 60 * 1000,
    };

    cached = current;
    current.config.catch(() => {
        if (cached === current) cached = null;
    });

    return current.config;
};

export const startOidcFlow = async (
    event: H3Event,
    settings: InstanceSettings,
    flow: Pick<OidcFlow, 'intent' | 'userId' | 'sessionId' | 'redirectTo'>,
) => {
    const config = await getOidcConfig(settings);
    const codeVerifier = client.randomPKCECodeVerifier();
    const state = client.randomState();
    const nonce = client.randomNonce();
    const value: OidcFlow = {
        ...flow,
        state,
        nonce,
        codeVerifier,
        expiresAt: Date.now() + 10 * 60 * 1000,
    };

    setCookie(event, FLOW_COOKIE, await encrypt(JSON.stringify(value)), {
        httpOnly: true,
        secure: getRequestProtocol(event, { xForwardedProto: true }) === 'https',
        sameSite: 'lax',
        path: FLOW_PATH,
        maxAge: 10 * 60,
    });

    return client.buildAuthorizationUrl(config, {
        redirect_uri: oidcRedirectUri(settings),
        scope: 'openid email profile',
        code_challenge: await client.calculatePKCECodeChallenge(codeVerifier),
        code_challenge_method: 'S256',
        state,
        nonce,
        ui_locales: requestLanguage(event),
        ...(flow.intent !== 'login' && { max_age: '0' }),
    }).href;
};

export const takeOidcFlow = async (event: H3Event) => {
    const cookie = getCookie(event, FLOW_COOKIE);
    if (!cookie) return null;

    deleteCookie(event, FLOW_COOKIE, { path: FLOW_PATH });

    const flow = await decrypt(cookie)
        .then((value) => JSON.parse(value) as OidcFlow)
        .catch(() => null);

    return flow && flow.expiresAt > Date.now() ? flow : null;
};

const stringClaim = (value: unknown) =>
    typeof value === 'string' && value.trim() ? value.trim() : null;

export const readOidcClaims = async (
    event: H3Event,
    settings: InstanceSettings,
    flow: OidcFlow,
): Promise<OidcClaims | null> => {
    try {
        const config = await getOidcConfig(settings);
        const currentUrl = new URL(oidcRedirectUri(settings));
        currentUrl.search = getRequestURL(event).search;

        const tokens = await client.authorizationCodeGrant(config, currentUrl, {
            pkceCodeVerifier: flow.codeVerifier,
            expectedState: flow.state,
            expectedNonce: flow.nonce,
            idTokenExpected: true,
        });

        const idClaims = tokens.claims()!;
        const userInfo = config.serverMetadata().userinfo_endpoint
            ? await client
                  .fetchUserInfo(config, tokens.access_token, idClaims.sub)
                  .catch(() => null)
            : null;
        const claims = { ...idClaims, ...userInfo };
        const email = stringClaim(claims.email)?.toLowerCase() ?? null;

        return {
            issuer: idClaims.iss,
            subject: idClaims.sub,
            email,
            emailVerified: !!email && String(claims.email_verified) === 'true',
            username:
                stringClaim(claims.preferred_username) ??
                stringClaim(claims.nickname) ??
                email?.split('@')[0] ??
                stringClaim(claims.name),
        };
    } catch (error: any) {
        consola.warn(
            `Single sign-on failed: ${error?.error_description ?? error?.message ?? error}`,
        );
        return null;
    }
};

export const oidcUsername = async (claims: OidcClaims) => {
    const candidate = (claims.username ?? '').replace(/\s+/g, ' ').slice(0, 24).trim();
    const base = candidate.length >= 3 ? candidate : 'user';

    for (let suffix = 1; suffix <= 50; suffix++) {
        const username =
            suffix === 1 ? base : `${base.slice(0, 24 - String(suffix).length)}${suffix}`;

        if (!(await prisma.user.count({ where: { username } }))) return username;
    }

    return `${base.slice(0, 17)}_${randomBytes(3).toString('hex')}`;
};
