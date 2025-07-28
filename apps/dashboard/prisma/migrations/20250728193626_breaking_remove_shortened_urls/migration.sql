/*
  Warnings:

  - The values [ShortenUrls] on the enum `UserPermission` will be removed. If these variants are still used in the database, this will fail.
  - You are about to drop the column `urlId` on the `View` table. All the data in the column will be lost.
  - You are about to drop the `Url` table. If the table is not empty, all the data it contains will be lost.

*/
-- AlterEnum
BEGIN;
CREATE TYPE "UserPermission_new" AS ENUM ('Admin', 'UploadFiles', 'TakeNotes', 'ShareCodes');
ALTER TABLE "User" ALTER COLUMN "permissions" TYPE "UserPermission_new"[] USING ("permissions"::text::"UserPermission_new"[]);
ALTER TYPE "UserPermission" RENAME TO "UserPermission_old";
ALTER TYPE "UserPermission_new" RENAME TO "UserPermission";
DROP TYPE "UserPermission_old";
COMMIT;

-- DropForeignKey
ALTER TABLE "Url" DROP CONSTRAINT "Url_authorId_fkey";

-- DropForeignKey
ALTER TABLE "View" DROP CONSTRAINT "View_urlId_fkey";

-- AlterTable
ALTER TABLE "View" DROP COLUMN "urlId";

-- DropTable
DROP TABLE "Url";
