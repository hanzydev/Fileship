-- Embeddings from the previous English-only models are not comparable with the new ones
UPDATE "File" SET "embedding" = NULL, "textEmbedding" = NULL;

DELETE FROM "AIJob"
WHERE "type"::text IN ('GenerateClipEmbedding', 'GenerateVideoEmbedding', 'GenerateTextEmbedding');
