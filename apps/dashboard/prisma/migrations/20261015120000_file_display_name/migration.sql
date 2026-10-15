-- AlterTable
ALTER TABLE "File" ADD COLUMN IF NOT EXISTS "name" TEXT;

UPDATE "File" SET "name" = "fileName" WHERE "name" IS NULL;

ALTER TABLE "File" ALTER COLUMN "name" SET NOT NULL;
