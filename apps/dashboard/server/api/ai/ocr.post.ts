export default defineEventHandler(async (event) => {
    const image = await readAIImage(event, 'ocr', 30);

    const text = await aiOcrImage(image)
        .then(ai.recognizeText)
        .catch(() => {
            throw apiError(422, 'errorsSystem.imageUnreadable');
        });

    return { text };
});
