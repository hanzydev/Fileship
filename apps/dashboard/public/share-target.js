const CACHE_NAME = 'fileship-share';
const SHARE_PATH = '/dashboard/share';
const MAX_AGE = 24 * 60 * 60 * 1000;

const pruneShared = async (cache, now) => {
    for (const request of await cache.keys()) {
        const response = await cache.match(request);

        if (!response || now - Number(response.headers.get('X-Shared-At')) > MAX_AGE)
            await cache.delete(request);
    }
};

const storeShared = async (request) => {
    const data = await request.formData();
    const cache = await caches.open(CACHE_NAME);
    const sharedAt = String(Date.now());
    const prefix = `${SHARE_PATH}/${crypto.randomUUID()}`;

    await pruneShared(cache, Number(sharedAt));

    const files = data.getAll('files').filter((file) => file instanceof File);

    for (const [index, file] of files.entries()) {
        await cache.put(
            `${prefix}/${index}`,
            new Response(file, {
                headers: {
                    'Content-Type': file.type || 'application/octet-stream',
                    'X-File-Name': encodeURIComponent(file.name),
                    'X-Last-Modified': String(file.lastModified),
                    'X-Shared-At': sharedAt,
                },
            }),
        );
    }

    const text = Object.fromEntries(
        ['title', 'text', 'url'].map((key) => [
            key,
            String(data.get(key) ?? '')
                .replace(/\r\n?/g, '\n')
                .trim(),
        ]),
    );

    if (text.title || text.text || text.url) {
        await cache.put(
            `${prefix}/text`,
            Response.json(text, { headers: { 'X-Shared-At': sharedAt } }),
        );
    }
};

self.addEventListener('fetch', (event) => {
    const url = new URL(event.request.url);

    if (
        event.request.method !== 'POST' ||
        url.origin !== self.location.origin ||
        url.pathname !== SHARE_PATH
    )
        return;

    event.respondWith(
        storeShared(event.request).then(
            () => Response.redirect(SHARE_PATH, 303),
            () => Response.redirect(`${SHARE_PATH}?failed=1`, 303),
        ),
    );
});
