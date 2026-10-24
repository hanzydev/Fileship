const MAX_TASKS = 20;
const KEEP_ALIVE_MS = 20_000;

const TITLES = {
    link: {
        copied: 'notify.shortenedCopied',
        done: 'notify.shortened',
        failed: 'notify.shortenFailed',
    },
    upload: {
        copied: 'notify.uploadedCopied',
        done: 'notify.uploaded',
        failed: 'notify.uploadFailed',
    },
};

export default defineBackground(() => {
    let tasks: Task[] = [];

    const loaded = tasksItem.getValue().then((saved) => {
        tasks = saved.map((task) =>
            task.status === 'running'
                ? { ...task, status: 'failed', error: t('errors.interrupted') }
                : task,
        );

        return tasksItem.setValue(tasks);
    });

    const saveTasks = (next: Task[]) => {
        tasks = next;
        return tasksItem.setValue(tasks);
    };

    const patchTask = (id: string, patch: Partial<Task>) =>
        saveTasks(tasks.map((task) => (task.id === id ? { ...task, ...patch } : task)));

    const notify = (title: string, message: string, url?: string) =>
        browser.notifications.create(url ? `link:${url}` : crypto.randomUUID(), {
            type: 'basic',
            iconUrl: browser.runtime.getURL('/icon/128.png'),
            title,
            message,
        });

    const runTask = async (
        kind: TaskKind,
        name: string,
        work: (onProgress: (sent: number, total: number) => void) => Promise<string>,
    ) => {
        await loaded;

        const id = crypto.randomUUID();
        const task: Task = { id, kind, name, status: 'running', progress: 0 };
        const titles = TITLES[kind === 'link' ? 'link' : 'upload'];
        const keepAlive = setInterval(() => browser.runtime.getPlatformInfo(), KEEP_ALIVE_MS);

        await saveTasks([task, ...tasks].slice(0, MAX_TASKS));

        try {
            const url = await work((sent, total) =>
                patchTask(id, { progress: total ? Math.round((sent / total) * 100) : 100 }),
            );
            const isCopied = await copyText(url).then(
                () => true,
                () => false,
            );

            await patchTask(id, { status: 'done', progress: 100, url });
            notify(t(isCopied ? titles.copied : titles.done), url, url);
        } catch (error) {
            const message = describeError(error, kind);

            await patchTask(id, { status: 'failed', error: message });
            notify(t(titles.failed), message);

            if (error instanceof ExtensionError && error.key === 'errors.notConfigured')
                browser.runtime.openOptionsPage();
        } finally {
            clearInterval(keepAlive);
        }
    };

    const getConnection = async () => {
        const settings = await settingsItem.getValue();
        if (!settings.instanceUrl || !settings.token)
            throw new ExtensionError('errors.notConfigured');

        const isGranted = await browser.permissions.contains({
            origins: [originPattern(settings.instanceUrl)],
        });

        if (!isGranted)
            throw new ExtensionError('errors.hostPermissionMissing', {
                origin: new URL(settings.instanceUrl).host,
            });

        const connection: Connection = {
            instanceUrl: settings.instanceUrl,
            token: settings.token,
            language: getLanguage(),
        };

        return { connection, settings };
    };

    const upload = async (
        blob: Blob,
        name: string,
        onProgress: (sent: number, total: number) => void,
    ) => {
        const { connection, settings } = await getConnection();
        const file = await uploadBlob(connection, { blob, name }, settings, onProgress);

        return file.url;
    };

    const shorten = async (url: string) => {
        if (!/^https?:\/\//i.test(url)) throw new ExtensionError('errors.pageNotSupported');

        const { connection } = await getConnection();
        return shortenUrl(connection, url);
    };

    const captureScreenshot = (windowId: number, rect: AreaRect | null, viewportWidth = 0) => {
        const name = screenshotName();

        runTask('screenshot', name, async (onProgress) => {
            const image = await captureTab(windowId);
            const blob = rect ? await cropImage(image, rect, viewportWidth) : image;

            return upload(blob, name, onProgress);
        });
    };

    const createMenus = async () => {
        await browser.contextMenus.removeAll();

        browser.contextMenus.create({
            id: 'uploadImage',
            title: t('menu.uploadImage'),
            contexts: ['image'],
        });

        browser.contextMenus.create({
            id: 'shortenLink',
            title: t('menu.shortenLink'),
            contexts: ['link'],
            targetUrlPatterns: ['http://*/*', 'https://*/*'],
        });

        browser.contextMenus.create({
            id: 'shortenPage',
            title: t('menu.shortenPage'),
            contexts: ['page'],
            documentUrlPatterns: ['http://*/*', 'https://*/*'],
        });
    };

    browser.runtime.onInstalled.addListener(createMenus);

    browser.runtime.onStartup.addListener(() => {
        createMenus();
        clearFiles();
    });

    browser.contextMenus.onClicked.addListener((info, tab) => {
        if (info.menuItemId === 'uploadImage' && info.srcUrl) {
            const name = imageName(info.srcUrl);
            const image = fetchImage(info.srcUrl, {
                url: info.frameUrl ?? info.pageUrl,
                tabId: tab?.id,
                frameId: info.frameId,
            });

            image.catch(() => {});

            runTask('image', name, async (onProgress) => {
                const blob = await image;
                return upload(blob, withImageExtension(name, blob.type), onProgress);
            });
        }

        if (info.menuItemId === 'shortenLink' && info.linkUrl)
            runTask('link', info.linkUrl, () => shorten(info.linkUrl!));

        const pageUrl = info.pageUrl ?? tab?.url;

        if (info.menuItemId === 'shortenPage' && pageUrl)
            runTask('link', pageUrl, () => shorten(pageUrl));
    });

    browser.notifications.onClicked.addListener((id) => {
        if (id.startsWith('link:')) browser.tabs.create({ url: id.slice(5) });
        browser.notifications.clear(id);
    });

    browser.runtime.onMessage.addListener((message: ExtensionMessage, sender, sendResponse) => {
        if (message.type === 'uploadFiles')
            for (const file of message.files)
                runTask('file', file.name, async (onProgress) => {
                    const blob = await readFile(file.id);
                    if (!blob) throw new ExtensionError('errors.fileMissing');

                    return upload(blob, file.name, onProgress).finally(() => removeFile(file.id));
                });

        if (message.type === 'shortenPage')
            runTask('link', message.url, () => shorten(message.url));

        if (message.type === 'captureVisible') captureScreenshot(message.windowId, null);

        if (message.type === 'areaSelected' && sender.tab)
            captureScreenshot(sender.tab.windowId, message.rect, message.viewportWidth);

        if (message.type === 'clearTasks')
            loaded.then(() => saveTasks(tasks.filter((task) => task.status === 'running')));

        if (message.type === 'captureArea') {
            browser.scripting
                .executeScript({ target: { tabId: message.tabId }, files: ['/area.js'] })
                .then(
                    () => sendResponse({}),
                    () => sendResponse({ error: t('errors.captureNotAllowed') }),
                );

            return true;
        }
    });
});
