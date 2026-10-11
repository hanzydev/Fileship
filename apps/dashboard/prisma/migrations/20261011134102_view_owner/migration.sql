-- AlterTable
ALTER TABLE "View" ADD COLUMN     "ownerId" TEXT;

-- Backfill owners of existing views
UPDATE "View" v SET "ownerId" = f."authorId" FROM "File" f WHERE v."fileId" = f."id";

-- CreateIndex
CREATE INDEX "View_createdAt_idx" ON "View"("createdAt");

-- CreateIndex
CREATE INDEX "View_ownerId_idx" ON "View"("ownerId");

-- AddForeignKey
ALTER TABLE "View" ADD CONSTRAINT "View_ownerId_fkey" FOREIGN KEY ("ownerId") REFERENCES "User"("id") ON DELETE CASCADE ON UPDATE CASCADE;
