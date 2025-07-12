/*
  Warnings:

  - The primary key for the `Stat` table will be changed. If it partially fails, the table could be left without primary key constraint.
  - The primary key for the `View` table will be changed. If it partially fails, the table could be left without primary key constraint.

*/
-- AlterTable
ALTER TABLE "Stat" DROP CONSTRAINT "Stat_pkey",
ALTER COLUMN "id" DROP DEFAULT,
ALTER COLUMN "id" SET DATA TYPE TEXT,
ADD CONSTRAINT "Stat_pkey" PRIMARY KEY ("id");
DROP SEQUENCE "Stat_id_seq";

-- AlterTable
ALTER TABLE "View" DROP CONSTRAINT "View_pkey",
ALTER COLUMN "id" DROP DEFAULT,
ALTER COLUMN "id" SET DATA TYPE TEXT,
ADD CONSTRAINT "View_pkey" PRIMARY KEY ("id");
DROP SEQUENCE "View_id_seq";
