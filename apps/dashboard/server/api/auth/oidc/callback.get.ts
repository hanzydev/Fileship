const resultPaths = {
    login: '/login?sso=',
    link: '/dashboard/settings/security?sso=',
    verify: '/sso?status=',
};

const userWithCount = { _count: { select: userCountSelect } } as const;

export default defineEventHandler(async (event) => {
    const flow = await takeOidcFlow(event);
    const query = getQuery(event);
    const intent = flow?.intent ?? 'login';
    const finish = (status: string) => sendRedirect(event, `${resultPaths[intent]}${status}`);

    const limited = await rateLimit(`oidcCallback:${getClientIp(event)}`, 30, 15 * 60 * 1000)
        .then(() => false)
        .catch(() => true);

    if (limited) return finish('tooManyAttempts');
    if (!flow || query.state !== flow.state) return finish('expired');
    if (query.error) return finish('denied');

    const settings = await getSettings();
    if (!isOidcConfigured(settings)) return finish('unavailable');

    const claims = await readOidcClaims(event, settings, flow);
    if (!claims) return finish('failed');

    const identityKey = { issuer_subject: { issuer: claims.issuer, subject: claims.subject } };
    const currentUser = event.context.user;

    if (flow.intent === 'verify') {
        if (!currentUser || currentUser.currentSessionId !== flow.sessionId) {
            return finish('expired');
        }

        const identity = await prisma.oidcIdentity.findUnique({ where: identityKey });
        if (identity?.userId !== currentUser.id) return finish('wrongAccount');

        await prisma.session.update({
            where: { id: currentUser.currentSessionId },
            data: { lastVerify: new Date() },
        });

        return finish('verified');
    }

    if (flow.intent === 'link') {
        if (!currentUser || currentUser.id !== flow.userId) return finish('expired');

        const identity = await prisma.oidcIdentity.findUnique({ where: identityKey });
        if (identity && identity.userId !== currentUser.id) return finish('alreadyLinked');

        const linked = await prisma
            .$transaction([
                prisma.oidcIdentity.deleteMany({
                    where: { userId: currentUser.id, issuer: claims.issuer },
                }),
                prisma.oidcIdentity.create({
                    data: {
                        issuer: claims.issuer,
                        subject: claims.subject,
                        email: claims.email,
                        userId: currentUser.id,
                    },
                }),
            ])
            .then(() => true)
            .catch(() => false);

        if (!linked) return finish('alreadyLinked');

        await createLog(event, {
            action: 'Link SSO',
            message: 'Linked a single sign-on account',
        });

        return finish('linked');
    }

    const identity = await prisma.oidcIdentity.findUnique({
        where: identityKey,
        include: { user: { include: userWithCount } },
    });

    if (identity) {
        if (identity.email !== claims.email) {
            await prisma.oidcIdentity.update({
                where: { id: identity.id },
                data: { email: claims.email },
            });
        }

        await startSession(event, identity.user, {
            action: 'Login',
            message: 'Logged in with single sign-on',
        });

        return sendRedirect(event, flow.redirectTo ?? '/dashboard');
    }

    const emailMatch =
        claims.emailVerified &&
        (await prisma.user.findUnique({
            where: { email: claims.email! },
            include: { ...userWithCount, oidcIdentities: { select: { issuer: true } } },
        }));

    if (emailMatch) {
        if (emailMatch.oidcIdentities.some(({ issuer }) => isSameIssuer(issuer, claims.issuer))) {
            return finish('conflict');
        }

        const linked = await prisma.oidcIdentity
            .create({
                data: {
                    issuer: claims.issuer,
                    subject: claims.subject,
                    email: claims.email,
                    userId: emailMatch.id,
                },
            })
            .then(() => true)
            .catch(() => false);

        if (!linked) return finish('conflict');

        await createLog(event, {
            action: 'Link SSO',
            message: 'Linked a single sign-on account by its confirmed email address',
            user: emailMatch,
        });

        await startSession(event, emailMatch, {
            action: 'Login',
            message: 'Logged in with single sign-on',
        });

        return sendRedirect(event, flow.redirectTo ?? '/dashboard');
    }

    if (!settings.oidcAutoRegister) return finish('noAccount');

    const createdUser = await createUser({
        username: await oidcUsername(claims),
        permissions: settings.registrationPermissions,
        limits: { usableSpace: settings.registrationUsableSpace },
        email: claims.emailVerified ? claims.email! : undefined,
        oidcIdentity: { issuer: claims.issuer, subject: claims.subject, email: claims.email },
    }).catch(() => null);

    if (!createdUser) return finish('failed');

    await startSession(
        event,
        await prisma.user.findUniqueOrThrow({
            where: { id: createdUser.id },
            include: userWithCount,
        }),
        { action: 'Register', message: 'Registered with single sign-on' },
    );

    return sendRedirect(event, flow.redirectTo ?? '/dashboard');
});
