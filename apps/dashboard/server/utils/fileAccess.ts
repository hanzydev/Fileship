const UUID = /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i;

export const isUuid = (value: string) => UUID.test(value);

export const fileLookups = (idOrName: string) =>
    isUuid(idOrName) ? [{ id: idOrName }, { fileName: idOrName }] : [{ fileName: idOrName }];

export const isHiddenFromVisitors = (file: { piiDetected: boolean; expiresAt: Date | null }) =>
    file.piiDetected || (!!file.expiresAt && file.expiresAt.getTime() <= Date.now());

export const visitorFileFields = {
    authorId: undefined,
    caption: undefined,
    piiReasons: undefined,
    piiOverride: undefined,
    piiDetected: undefined,
    maxViews: undefined,
    expiresAt: undefined,
    storageId: undefined,
    aiAnalyzedAt: undefined,
    embeddingModel: undefined,
    textEmbeddingModel: undefined,
    contentHash: undefined,
};
