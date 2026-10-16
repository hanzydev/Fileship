-- AlterTable
ALTER TABLE "User" ADD COLUMN     "uploadTokenEncrypted" TEXT,
ADD COLUMN     "uploadTokenHash" TEXT;

-- CreateIndex
CREATE UNIQUE INDEX "User_uploadTokenHash_key" ON "User"("uploadTokenHash");
