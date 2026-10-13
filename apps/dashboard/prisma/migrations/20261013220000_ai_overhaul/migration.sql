-- Jobs are rebuilt by ai:syncJobs with the new job types
DELETE FROM "AIJob";

-- AlterEnum
BEGIN;
CREATE TYPE "AIJobType_new" AS ENUM ('AnalyzeImage', 'AnalyzeVideo', 'AnalyzeText', 'DetectPII');
ALTER TABLE "AIJob" ALTER COLUMN "type" TYPE "AIJobType_new" USING ("type"::text::"AIJobType_new");
ALTER TYPE "AIJobType" RENAME TO "AIJobType_old";
ALTER TYPE "AIJobType_new" RENAME TO "AIJobType";
DROP TYPE "AIJobType_old";
COMMIT;

-- DropIndex
DROP INDEX IF EXISTS "AIJob_userId_status_createdAt_idx";

-- AlterTable
ALTER TABLE "File" ADD COLUMN IF NOT EXISTS "aiAnalyzedAt" TIMESTAMP(3),
ADD COLUMN IF NOT EXISTS "embeddingModel" TEXT,
ADD COLUMN IF NOT EXISTS "textEmbeddingModel" TEXT;

-- AlterTable
ALTER TABLE "AIJob" ALTER COLUMN "fileId" SET NOT NULL;

-- One active job per file and type
CREATE UNIQUE INDEX IF NOT EXISTS "AIJob_fileId_type_active_key" ON "AIJob"("fileId", "type") WHERE "status" IN ('Pending', 'Processing');

-- New embedding models and OCR engine: everything is analyzed again
UPDATE "File" SET "embedding" = NULL, "textEmbedding" = NULL, "ocrText" = NULL;
