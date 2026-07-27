export const parseSseStream = async <T = unknown>(
    stream: ReadableStream<Uint8Array>,
    onMessage: (data: T) => void,
) => {
    const reader = stream.getReader();
    const decoder = new TextDecoder();
    let buffer = '';

    while (true) {
        const { done, value } = await reader.read();
        if (done) break;

        buffer += decoder.decode(value, { stream: true });
        const lines = buffer.split('\n');
        buffer = lines.pop() || '';

        for (const line of lines) {
            if (line.startsWith('data:')) {
                try {
                    onMessage(JSON.parse(line.slice(5).trim()));
                } catch {
                    //
                }
            }
        }
    }

    if (buffer.startsWith('data:')) {
        try {
            onMessage(JSON.parse(buffer.slice(5).trim()));
        } catch {
            //
        }
    }
};
