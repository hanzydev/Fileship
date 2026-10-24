-- CreateEnum
CREATE TYPE "BackupFrequency" AS ENUM ('Off', 'Daily', 'Weekly');

-- CreateEnum
CREATE TYPE "BackupRunStatus" AS ENUM ('Created', 'LimitReached', 'Failed');

-- AlterTable
ALTER TABLE "Backup" ADD COLUMN     "automatic" BOOLEAN NOT NULL DEFAULT false;

-- CreateTable
CREATE TABLE "BackupSchedule" (
    "userId" TEXT NOT NULL,
    "frequency" "BackupFrequency" NOT NULL,
    "hour" INTEGER NOT NULL,
    "weekday" INTEGER NOT NULL,
    "timezone" TEXT NOT NULL,
    "keep" INTEGER NOT NULL,
    "nextRunAt" TIMESTAMP(3),
    "lastRunAt" TIMESTAMP(3),
    "lastRunStatus" "BackupRunStatus",

    CONSTRAINT "BackupSchedule_pkey" PRIMARY KEY ("userId")
);

-- CreateIndex
CREATE INDEX "BackupSchedule_nextRunAt_idx" ON "BackupSchedule"("nextRunAt");

-- AddForeignKey
ALTER TABLE "BackupSchedule" ADD CONSTRAINT "BackupSchedule_userId_fkey" FOREIGN KEY ("userId") REFERENCES "User"("id") ON DELETE CASCADE ON UPDATE CASCADE;
