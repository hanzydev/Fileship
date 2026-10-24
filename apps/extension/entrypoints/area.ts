const TOKENS = {
    '--primary': 'oklch(0.585 0.233 277.117)',
    '--background': 'oklch(0.145 0 0 / 85%)',
    '--foreground': 'oklch(0.985 0 0)',
    '--overlay': 'oklch(0 0 0 / 40%)',
};

export default defineUnlistedScript(() => {
    document.querySelector('fileship-area')?.remove();

    const host = document.createElement('fileship-area');
    const root = host.attachShadow({ mode: 'closed' });
    const selection = document.createElement('div');
    const hint = document.createElement('div');

    Object.entries(TOKENS).forEach(([name, value]) => host.style.setProperty(name, value));

    Object.assign(host.style, {
        all: 'initial',
        position: 'fixed',
        inset: '0',
        zIndex: '2147483647',
        cursor: 'crosshair',
        userSelect: 'none',
        background: 'var(--overlay)',
    });

    Object.assign(selection.style, {
        display: 'none',
        position: 'fixed',
        boxSizing: 'border-box',
        border: '1px solid var(--primary)',
        boxShadow: '0 0 0 100vmax var(--overlay)',
    });

    Object.assign(hint.style, {
        position: 'fixed',
        top: '16px',
        left: '50%',
        transform: 'translateX(-50%)',
        padding: '8px 16px',
        borderRadius: '9999px',
        background: 'var(--background)',
        color: 'var(--foreground)',
        font: '500 13px/1.4 system-ui, sans-serif',
        whiteSpace: 'nowrap',
        pointerEvents: 'none',
    });

    hint.textContent = t('area.hint');
    root.append(selection, hint);
    document.documentElement.append(host);

    let start: { x: number; y: number } | null = null;
    let rect: AreaRect = { x: 0, y: 0, width: 0, height: 0 };

    const handleKeyDown = (event: KeyboardEvent) => {
        if (event.key !== 'Escape') return;

        event.preventDefault();
        event.stopPropagation();
        close();
    };

    const close = () => {
        host.remove();
        window.removeEventListener('keydown', handleKeyDown, true);
    };

    const finish = async (area: AreaRect | null) => {
        close();

        await new Promise((resolve) => requestAnimationFrame(() => requestAnimationFrame(resolve)));

        browser.runtime.sendMessage({
            type: 'areaSelected',
            rect: area,
            viewportWidth: window.innerWidth,
        } satisfies ExtensionMessage);
    };

    host.addEventListener('pointerdown', (event) => {
        if (event.button !== 0) return close();

        start = { x: event.clientX, y: event.clientY };
        host.setPointerCapture(event.pointerId);
        hint.style.display = 'none';
    });

    host.addEventListener('pointermove', (event) => {
        if (!start) return;

        rect = {
            x: Math.min(start.x, event.clientX),
            y: Math.min(start.y, event.clientY),
            width: Math.abs(event.clientX - start.x),
            height: Math.abs(event.clientY - start.y),
        };

        host.style.background = 'transparent';

        Object.assign(selection.style, {
            display: 'block',
            left: `${rect.x}px`,
            top: `${rect.y}px`,
            width: `${rect.width}px`,
            height: `${rect.height}px`,
        });
    });

    host.addEventListener('pointerup', () => {
        if (start) finish(rect.width > 3 && rect.height > 3 ? rect : null);
    });

    host.addEventListener('contextmenu', (event) => {
        event.preventDefault();
        close();
    });

    window.addEventListener('keydown', handleKeyDown, true);
});
