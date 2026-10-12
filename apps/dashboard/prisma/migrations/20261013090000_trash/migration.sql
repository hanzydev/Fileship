-- AlterTable
ALTER TABLE "File" ADD COLUMN     "deletedAt" TIMESTAMP(3);

-- AlterTable
ALTER TABLE "Note" ADD COLUMN     "deletedAt" TIMESTAMP(3);

-- AlterTable
ALTER TABLE "Folder" ADD COLUMN     "deletedAt" TIMESTAMP(3);

-- CreateIndex
CREATE INDEX "File_authorId_deletedAt_idx" ON "File"("authorId", "deletedAt");

-- CreateIndex
CREATE INDEX "Note_authorId_deletedAt_idx" ON "Note"("authorId", "deletedAt");

-- CreateIndex
CREATE INDEX "Folder_authorId_deletedAt_idx" ON "Folder"("authorId", "deletedAt");

