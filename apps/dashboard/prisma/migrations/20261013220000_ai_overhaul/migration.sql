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
DROP INDEX "AIJob_userId_status_createdAt_idx";

-- AlterTable
ALTER TABLE "File" ADD COLUMN     "aiAnalyzedAt" TIMESTAMP(3),
ADD COLUMN     "embeddingModel" TEXT,
ADD COLUMN     "textEmbeddingModel" TEXT;

-- AlterTable
ALTER TABLE "AIJob" ALTER COLUMN "fileId" SET NOT NULL;

-- One active job per file and type
CREATE UNIQUE INDEX "AIJob_fileId_type_active_key" ON "AIJob"("fileId", "type") WHERE "status" IN ('Pending', 'Processing');

-- New embedding models and OCR engine: everything is analyzed again
UPDATE "File" SET "embedding" = NULL, "textEmbedding" = NULL, "ocrText" = NULL;
