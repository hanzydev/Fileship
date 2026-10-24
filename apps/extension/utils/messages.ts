export type AreaRect = {
    x: number;
    y: number;
    width: number;
    height: number;
};

export type ExtensionMessage =
    | { type: 'uploadFiles'; files: { id: string; name: string }[] }
    | { type: 'shortenPage'; url: string }
    | { type: 'captureVisible'; windowId: number }
    | { type: 'captureArea'; tabId: number }
    | { type: 'areaSelected'; rect: AreaRect | null; viewportWidth: number }
    | { type: 'clearTasks' }
    | { type: 'copy'; text: string };

export const sendMessage = <T = void>(message: ExtensionMessage) =>
    browser.runtime.sendMessage(message) as Promise<T>;
