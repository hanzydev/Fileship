browser.runtime.onMessage.addListener((message: ExtensionMessage, _sender, sendResponse) => {
    if (message.type !== 'copy') return;

    const textarea = document.createElement('textarea');

    textarea.value = message.text;
    document.body.append(textarea);
    textarea.select();

    sendResponse(document.execCommand('copy'));
    textarea.remove();
});
