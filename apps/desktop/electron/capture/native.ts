import koffi from 'koffi';

export type Rect = { x: number; y: number; width: number; height: number };

export type WindowRect = Rect & { title: string; controls: Rect[] };

const user32 = koffi.load('user32.dll');
const gdi32 = koffi.load('gdi32.dll');
const dwmapi = koffi.load('dwmapi.dll');
const dxgi = koffi.load('dxgi.dll');

koffi.struct('RECT', {
    left: 'long',
    top: 'long',
    right: 'long',
    bottom: 'long',
});

const POINT = koffi.struct('POINT', { x: 'long', y: 'long' });

const CURSORINFO = koffi.struct('CURSORINFO', {
    cbSize: 'uint32',
    flags: 'uint32',
    hCursor: 'void *',
    ptScreenPos: POINT,
});

koffi.struct('ICONINFO', {
    fIcon: 'bool',
    xHotspot: 'uint32',
    yHotspot: 'uint32',
    hbmMask: 'void *',
    hbmColor: 'void *',
});

const BITMAPINFOHEADER = koffi.struct('BITMAPINFOHEADER', {
    biSize: 'uint32',
    biWidth: 'int32',
    biHeight: 'int32',
    biPlanes: 'uint16',
    biBitCount: 'uint16',
    biCompression: 'uint32',
    biSizeImage: 'uint32',
    biXPelsPerMeter: 'int32',
    biYPelsPerMeter: 'int32',
    biClrUsed: 'uint32',
    biClrImportant: 'uint32',
});

koffi.struct('GUID', {
    Data1: 'uint32',
    Data2: 'uint16',
    Data3: 'uint16',
    Data4: koffi.array('uint8', 8),
});

koffi.struct('DXGI_OUTPUT_DESC', {
    DeviceName: koffi.array('char16_t', 32, 'String'),
    DesktopCoordinates: 'RECT',
    AttachedToDesktop: 'int',
    Rotation: 'int',
    Monitor: 'void *',
});

const CreateDXGIFactory1 = dxgi.func(
    'long __stdcall CreateDXGIFactory1(GUID *riid, _Out_ void **ppFactory)',
);
const EnumAdapters1 = koffi.proto(
    'long __stdcall EnumAdapters1(void *self, uint32 index, _Out_ void **adapter)',
);
const EnumOutputs = koffi.proto(
    'long __stdcall EnumOutputs(void *self, uint32 index, _Out_ void **output)',
);
const GetOutputDesc = koffi.proto(
    'long __stdcall GetOutputDesc(void *self, _Out_ DXGI_OUTPUT_DESC *desc)',
);
const Release = koffi.proto('uint32 __stdcall Release(void *self)');

const IID_IDXGIFactory1 = {
    Data1: 0x770aae78,
    Data2: 0xf26f,
    Data3: 0x4dba,
    Data4: [0xa8, 0x29, 0x25, 0x3c, 0x83, 0xd1, 0xb3, 0x87],
};

const EnumWindowsProc = koffi.proto('bool __stdcall EnumWindowsProc(void *hwnd, intptr_t lParam)');

const GetDC = user32.func('void * __stdcall GetDC(void *hWnd)');
const ReleaseDC = user32.func('int __stdcall ReleaseDC(void *hWnd, void *hDC)');
const GetCursorInfo = user32.func('bool __stdcall GetCursorInfo(_Inout_ CURSORINFO *pci)');
const GetIconInfo = user32.func(
    'bool __stdcall GetIconInfo(void *hIcon, _Out_ ICONINFO *piconinfo)',
);
const DrawIconEx = user32.func(
    'bool __stdcall DrawIconEx(void *hdc, int x, int y, void *hIcon, int cx, int cy, uint32 step, void *hbr, uint32 flags)',
);
const EnumWindows = user32.func(
    'bool __stdcall EnumWindows(EnumWindowsProc *lpEnumFunc, intptr_t lParam)',
);
const EnumChildWindows = user32.func(
    'bool __stdcall EnumChildWindows(void *hWndParent, EnumWindowsProc *lpEnumFunc, intptr_t lParam)',
);
const IsWindowVisible = user32.func('bool __stdcall IsWindowVisible(void *hWnd)');
const IsIconic = user32.func('bool __stdcall IsIconic(void *hWnd)');
const GetWindowLongPtrW = user32.func(
    'intptr_t __stdcall GetWindowLongPtrW(void *hWnd, int nIndex)',
);
const GetWindowTextW = user32.func(
    'int __stdcall GetWindowTextW(void *hWnd, _Out_ char16_t *lpString, int nMaxCount)',
);
const GetWindowRect = user32.func('bool __stdcall GetWindowRect(void *hWnd, _Out_ RECT *lpRect)');
const GetForegroundWindow = user32.func('void * __stdcall GetForegroundWindow()');

const CreateCompatibleDC = gdi32.func('void * __stdcall CreateCompatibleDC(void *hdc)');
const CreateCompatibleBitmap = gdi32.func(
    'void * __stdcall CreateCompatibleBitmap(void *hdc, int cx, int cy)',
);
const SelectObject = gdi32.func('void * __stdcall SelectObject(void *hdc, void *h)');
const BitBlt = gdi32.func(
    'bool __stdcall BitBlt(void *hdc, int x, int y, int cx, int cy, void *hdcSrc, int x1, int y1, uint32 rop)',
);
const GetDIBits = gdi32.func(
    'int __stdcall GetDIBits(void *hdc, void *hbm, uint32 start, uint32 cLines, _Out_ uint8_t *lpvBits, _Inout_ BITMAPINFOHEADER *lpbmi, uint32 usage)',
);
const DeleteObject = gdi32.func('bool __stdcall DeleteObject(void *ho)');
const DeleteDC = gdi32.func('bool __stdcall DeleteDC(void *hdc)');

const DwmGetWindowAttribute = dwmapi.func(
    'long __stdcall DwmGetWindowAttribute(void *hwnd, uint32 dwAttribute, _Out_ void *pvAttribute, uint32 cbAttribute)',
);

const SRCCOPY = 0x00cc0020;
const CAPTUREBLT = 0x40000000;
const DI_NORMAL = 0x0003;
const CURSOR_SHOWING = 0x00000001;
const GWL_EXSTYLE = -20;
const WS_EX_TOOLWINDOW = 0x00000080;
const WS_EX_NOACTIVATE = 0x08000000;
const DWMWA_EXTENDED_FRAME_BOUNDS = 9;
const DWMWA_CLOAKED = 14;
const MAX_CONTROLS = 3000;

const drawCursor = (hdc: unknown, origin: { x: number; y: number }) => {
    const info = {
        cbSize: koffi.sizeof(CURSORINFO),
        flags: 0,
        hCursor: null,
        ptScreenPos: { x: 0, y: 0 },
    };
    if (!GetCursorInfo(info) || !(info.flags & CURSOR_SHOWING) || !info.hCursor) return;

    const icon = {} as { xHotspot: number; yHotspot: number; hbmMask: unknown; hbmColor: unknown };
    const hasIcon = GetIconInfo(info.hCursor, icon);

    DrawIconEx(
        hdc,
        info.ptScreenPos.x - origin.x - (hasIcon ? icon.xHotspot : 0),
        info.ptScreenPos.y - origin.y - (hasIcon ? icon.yHotspot : 0),
        info.hCursor,
        0,
        0,
        0,
        null,
        DI_NORMAL,
    );

    if (hasIcon) {
        if (icon.hbmMask) DeleteObject(icon.hbmMask);
        if (icon.hbmColor) DeleteObject(icon.hbmColor);
    }
};

export const captureScreen = (rect: Rect, options: { cursor?: boolean } = {}) => {
    const screenDC = GetDC(null);
    const memoryDC = CreateCompatibleDC(screenDC);
    const bitmap = CreateCompatibleBitmap(screenDC, rect.width, rect.height);
    const previous = SelectObject(memoryDC, bitmap);

    try {
        BitBlt(
            memoryDC,
            0,
            0,
            rect.width,
            rect.height,
            screenDC,
            rect.x,
            rect.y,
            SRCCOPY | CAPTUREBLT,
        );

        if (options.cursor) drawCursor(memoryDC, rect);

        const data = Buffer.alloc(rect.width * rect.height * 4);

        GetDIBits(
            memoryDC,
            bitmap,
            0,
            rect.height,
            data,
            {
                biSize: koffi.sizeof(BITMAPINFOHEADER),
                biWidth: rect.width,
                biHeight: -rect.height,
                biPlanes: 1,
                biBitCount: 32,
                biCompression: 0,
                biSizeImage: 0,
                biXPelsPerMeter: 0,
                biYPelsPerMeter: 0,
                biClrUsed: 0,
                biClrImportant: 0,
            },
            0,
        );

        for (let i = 3; i < data.length; i += 4) data[i] = 255;

        return data;
    } finally {
        SelectObject(memoryDC, previous);
        DeleteObject(bitmap);
        DeleteDC(memoryDC);
        ReleaseDC(null, screenDC);
    }
};

const readTitle = (hwnd: unknown) => {
    const buffer = Buffer.alloc(512);
    const length = GetWindowTextW(hwnd, buffer, 256);
    return buffer.toString('utf16le', 0, length * 2);
};

const readFrame = (hwnd: unknown): Rect | null => {
    const rect = Buffer.alloc(16);

    if (DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, rect, 16) !== 0) {
        const fallback = { left: 0, top: 0, right: 0, bottom: 0 };
        if (!GetWindowRect(hwnd, fallback)) return null;
        rect.writeInt32LE(fallback.left, 0);
        rect.writeInt32LE(fallback.top, 4);
        rect.writeInt32LE(fallback.right, 8);
        rect.writeInt32LE(fallback.bottom, 12);
    }

    const left = rect.readInt32LE(0);
    const top = rect.readInt32LE(4);
    const width = rect.readInt32LE(8) - left;
    const height = rect.readInt32LE(12) - top;

    return width > 0 && height > 0 ? { x: left, y: top, width, height } : null;
};

const isCloaked = (hwnd: unknown) => {
    const value = Buffer.alloc(4);
    return (
        DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, value, 4) === 0 && value.readUInt32LE(0) !== 0
    );
};

const readControlRect = (hwnd: unknown): Rect | null => {
    if (!IsWindowVisible(hwnd)) return null;

    const rect = { left: 0, top: 0, right: 0, bottom: 0 };
    if (!GetWindowRect(hwnd, rect)) return null;

    const width = rect.right - rect.left;
    const height = rect.bottom - rect.top;

    return width > 4 && height > 4 ? { x: rect.left, y: rect.top, width, height } : null;
};

export const listWindows = (ignored: bigint[], options: { controls?: boolean } = {}) => {
    const windows: WindowRect[] = [];
    let controlCount = 0;

    const handleWindow = (hwnd: unknown) => {
        if (ignored.includes(koffi.address(hwnd))) return true;
        if (!IsWindowVisible(hwnd) || IsIconic(hwnd) || isCloaked(hwnd)) return true;

        const exStyle = Number(GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
        if (exStyle & WS_EX_TOOLWINDOW || exStyle & WS_EX_NOACTIVATE) return true;

        const frame = readFrame(hwnd);
        if (!frame) return true;

        const controls: Rect[] = [];

        if (options.controls && controlCount < MAX_CONTROLS) {
            const handleChild = (child: unknown) => {
                const rect = readControlRect(child);

                if (rect && controlCount < MAX_CONTROLS) {
                    controls.push(rect);
                    controlCount++;
                }

                return controlCount < MAX_CONTROLS;
            };

            const childCallback = koffi.register(handleChild, koffi.pointer(EnumWindowsProc));

            try {
                EnumChildWindows(hwnd, childCallback, 0);
            } finally {
                koffi.unregister(childCallback);
            }
        }

        windows.push({ ...frame, title: readTitle(hwnd), controls });
        return true;
    };

    const callback = koffi.register(handleWindow, koffi.pointer(EnumWindowsProc));

    try {
        EnumWindows(callback, 0);
    } finally {
        koffi.unregister(callback);
    }

    return windows;
};

export const foregroundWindow = (ignored: bigint[]) => {
    const hwnd = GetForegroundWindow();
    if (!hwnd || ignored.includes(koffi.address(hwnd))) return null;

    const frame = readFrame(hwnd);
    return frame ? { ...frame, title: readTitle(hwnd) } : null;
};

const method = (object: unknown, index: number, proto: ReturnType<typeof koffi.proto>) => {
    const table = koffi.decode(object, 'void *');
    const fn = koffi.decode(table, index * koffi.sizeof('void *'), 'void *');
    return (...args: unknown[]) => koffi.call(fn, proto, object, ...args) as number;
};

export type DisplayOutput = { adapter: number; output: number; rect: Rect };

export const listOutputs = () => {
    const outputs: DisplayOutput[] = [];
    const factoryOut: unknown[] = [null];
    if (CreateDXGIFactory1(IID_IDXGIFactory1, factoryOut) < 0) return outputs;

    const factory = factoryOut[0];

    for (let adapterIndex = 0; ; adapterIndex++) {
        const adapterOut: unknown[] = [null];
        if (method(factory, 12, EnumAdapters1)(adapterIndex, adapterOut) < 0) break;

        const adapter = adapterOut[0];

        for (let outputIndex = 0; ; outputIndex++) {
            const outputOut: unknown[] = [null];
            if (method(adapter, 7, EnumOutputs)(outputIndex, outputOut) < 0) break;

            const output = outputOut[0];
            const desc = {} as {
                DesktopCoordinates: { left: number; top: number; right: number; bottom: number };
            };

            if (method(output, 7, GetOutputDesc)(desc) >= 0) {
                const { left, top, right, bottom } = desc.DesktopCoordinates;
                outputs.push({
                    adapter: adapterIndex,
                    output: outputIndex,
                    rect: { x: left, y: top, width: right - left, height: bottom - top },
                });
            }

            method(output, 2, Release)();
        }

        method(adapter, 2, Release)();
    }

    method(factory, 2, Release)();
    return outputs;
};

const kernel32 = koffi.load('kernel32.dll');
const GetCurrentThreadId = kernel32.func('uint32 __stdcall GetCurrentThreadId()');
const GetWindowThreadProcessId = user32.func(
    'uint32 __stdcall GetWindowThreadProcessId(void *hWnd, _Out_ uint32 *lpdwProcessId)',
);
const AttachThreadInput = user32.func(
    'bool __stdcall AttachThreadInput(uint32 idAttach, uint32 idAttachTo, bool fAttach)',
);
const SetForegroundWindow = user32.func('bool __stdcall SetForegroundWindow(intptr_t hWnd)');
const BringWindowToTop = user32.func('bool __stdcall BringWindowToTop(intptr_t hWnd)');
const SetFocus = user32.func('intptr_t __stdcall SetFocus(intptr_t hWnd)');

export const forceForeground = (handle: bigint) => {
    const foreground = GetForegroundWindow();
    const current = GetCurrentThreadId();
    const target = foreground ? GetWindowThreadProcessId(foreground, null) : 0;
    const shouldAttach = target !== 0 && target !== current;

    if (shouldAttach) AttachThreadInput(current, target, true);

    try {
        BringWindowToTop(handle);
        SetForegroundWindow(handle);
        SetFocus(handle);
    } finally {
        if (shouldAttach) AttachThreadInput(current, target, false);
    }
};
