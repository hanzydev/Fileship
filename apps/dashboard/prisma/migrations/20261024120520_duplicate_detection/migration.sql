-- AlterTable
ALTER TABLE "File" ADD COLUMN IF NOT EXISTS "contentHash" TEXT;

-- CreateIndex
CREATE INDEX IF NOT EXISTS "File_authorId_size_idx" ON "File"("authorId", "size");

-- CreateIndex
CREATE INDEX IF NOT EXISTS "File_authorId_contentHash_idx" ON "File"("authorId", "contentHash");
