-- CreateTable
CREATE TABLE "Instance" (
    "id" TEXT NOT NULL,
    "firstSeenAt" TIMESTAMP(3) NOT NULL,
    "lastSeenAt" TIMESTAMP(3) NOT NULL,
    "version" TEXT NOT NULL DEFAULT '',
    "users" INTEGER NOT NULL DEFAULT 0,
    "files" INTEGER NOT NULL DEFAULT 0,
    "storage" BIGINT NOT NULL DEFAULT 0,
    "report" JSONB NOT NULL DEFAULT '{}',

    CONSTRAINT "Instance_pkey" PRIMARY KEY ("id")
);

-- CreateTable
CREATE TABLE "Report" (
    "id" TEXT NOT NULL,
    "instanceId" TEXT NOT NULL,
    "day" DATE NOT NULL,
    "receivedAt" TIMESTAMP(3) NOT NULL,
    "sentAt" TIMESTAMP(3),
    "legacy" BOOLEAN NOT NULL DEFAULT false,
    "payload" JSONB NOT NULL,

    CONSTRAINT "Report_pkey" PRIMARY KEY ("id")
);

-- CreateTable
CREATE TABLE "DailyStat" (
    "day" DATE NOT NULL,
    "instances" INTEGER NOT NULL,
    "users" INTEGER NOT NULL,
    "files" INTEGER NOT NULL,
    "storage" BIGINT NOT NULL,
    "links" INTEGER NOT NULL,
    "versions" JSONB NOT NULL,
    "data" JSONB NOT NULL,
    "updatedAt" TIMESTAMP(3) NOT NULL,

    CONSTRAINT "DailyStat_pkey" PRIMARY KEY ("day")
);

-- CreateTable
CREATE TABLE "Session" (
    "id" TEXT NOT NULL,
    "tokenHash" TEXT NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "expiresAt" TIMESTAMP(3) NOT NULL,

    CONSTRAINT "Session_pkey" PRIMARY KEY ("id")
);

-- CreateIndex
CREATE INDEX "Instance_lastSeenAt_idx" ON "Instance"("lastSeenAt");

-- CreateIndex
CREATE INDEX "Instance_version_idx" ON "Instance"("version");

-- CreateIndex
CREATE INDEX "Report_day_idx" ON "Report"("day");

-- CreateIndex
CREATE UNIQUE INDEX "Report_instanceId_day_key" ON "Report"("instanceId", "day");

-- CreateIndex
CREATE UNIQUE INDEX "Session_tokenHash_key" ON "Session"("tokenHash");

-- CreateIndex
CREATE INDEX "Session_expiresAt_idx" ON "Session"("expiresAt");

-- AddForeignKey
ALTER TABLE "Report" ADD CONSTRAINT "Report_instanceId_fkey" FOREIGN KEY ("instanceId") REFERENCES "Instance"("id") ON DELETE CASCADE ON UPDATE CASCADE;
