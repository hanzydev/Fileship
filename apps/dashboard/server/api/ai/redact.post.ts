export default defineEventHandler(async (event) => {
    const input = await readAIImage(event, 'redact', 60);

    const regions = await aiRedactImage(input)
        .then(async ({ image, scanned, original }) => {
            const { lines } = await ai.readLines(image);
            const entities = await ai.findPrivacyEntities(
                lines.map((line) => line.text).join('\n'),
            );

            return findRedactions(lines, entities, scanned, original);
        })
        .catch(() => {
            throw apiError(422, 'errorsSystem.imageUnreadable');
        });

    return { regions };
});
