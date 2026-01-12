-- CreateEnum
CREATE TYPE "AIJobType" AS ENUM ('GenerateClipEmbedding', 'GenerateVideoEmbedding', 'GenerateOcrText', 'GenerateTextEmbedding', 'GenerateImageCaption', 'DetectPII');

-- CreateEnum
CREATE TYPE "AIJobStatus" AS ENUM ('Pending', 'Processing', 'Completed', 'Failed');

-- AlterTable
ALTER TABLE "File" ADD COLUMN     "caption" TEXT,
ADD COLUMN     "ocrText" TEXT,
ADD COLUMN     "piiDetected" BOOLEAN NOT NULL DEFAULT false,
ADD COLUMN     "piiReasons" TEXT[],
ADD COLUMN     "textEmbedding" JSONB;

-- AlterTable
ALTER TABLE "User" ADD COLUMN     "aiSettings" JSONB NOT NULL DEFAULT '{}';

-- CreateTable
CREATE TABLE "AIJob" (
    "id" TEXT NOT NULL,
    "type" "AIJobType" NOT NULL,
    "status" "AIJobStatus" NOT NULL DEFAULT 'Pending',
    "attempts" INTEGER NOT NULL DEFAULT 0,
    "error" TEXT,
    "fileId" TEXT,
    "userId" TEXT NOT NULL,
    "lockedAt" TIMESTAMP(3),
    "lockedBy" TEXT,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL,

    CONSTRAINT "AIJob_pkey" PRIMARY KEY ("id")
);

-- CreateIndex
CREATE UNIQUE INDEX "AIJob_id_key" ON "AIJob"("id");

-- CreateIndex
CREATE INDEX "AIJob_status_createdAt_idx" ON "AIJob"("status", "createdAt");

-- CreateIndex
CREATE INDEX "AIJob_userId_status_createdAt_idx" ON "AIJob"("userId", "status", "createdAt");

-- CreateIndex
CREATE INDEX "AIJob_fileId_idx" ON "AIJob"("fileId");

-- AddForeignKey
ALTER TABLE "AIJob" ADD CONSTRAINT "AIJob_fileId_fkey" FOREIGN KEY ("fileId") REFERENCES "File"("id") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "AIJob" ADD CONSTRAINT "AIJob_userId_fkey" FOREIGN KEY ("userId") REFERENCES "User"("id") ON DELETE CASCADE ON UPDATE CASCADE;
