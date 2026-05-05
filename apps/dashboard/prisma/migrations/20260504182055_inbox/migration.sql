-- CreateTable
CREATE TABLE "Inbox" (
    "id" TEXT NOT NULL,
    "userId" TEXT NOT NULL,
    "folderId" TEXT NOT NULL,
    "enabled" BOOLEAN NOT NULL DEFAULT false,
    "password" TEXT,
    "slug" TEXT,

    CONSTRAINT "Inbox_pkey" PRIMARY KEY ("id")
);

-- CreateIndex
CREATE UNIQUE INDEX "Inbox_slug_key" ON "Inbox"("slug");

-- CreateIndex
CREATE UNIQUE INDEX "Inbox_id_key" ON "Inbox"("id");

-- CreateIndex
CREATE UNIQUE INDEX "Inbox_userId_key" ON "Inbox"("userId");

-- CreateIndex
CREATE UNIQUE INDEX "Inbox_folderId_key" ON "Inbox"("folderId");

-- AddForeignKey
ALTER TABLE "Inbox" ADD CONSTRAINT "Inbox_userId_fkey" FOREIGN KEY ("userId") REFERENCES "User"("id") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "Inbox" ADD CONSTRAINT "Inbox_folderId_fkey" FOREIGN KEY ("folderId") REFERENCES "Folder"("id") ON DELETE CASCADE ON UPDATE CASCADE;

DO $$
DECLARE
    user_record RECORD;
    new_folder_id TEXT;
    new_inbox_id TEXT;
BEGIN
    FOR user_record IN SELECT id FROM "User" WHERE id NOT IN (SELECT "userId" FROM "Inbox") LOOP
        new_folder_id := gen_random_uuid()::text;
        new_inbox_id := gen_random_uuid()::text;

        INSERT INTO "Folder" ("id", "name", "public", "createdAt", "authorId")
        VALUES (new_folder_id, 'Inbox', false, NOW(), user_record.id);

        INSERT INTO "Inbox" ("id", "userId", "folderId", "enabled")
        VALUES (new_inbox_id, user_record.id, new_folder_id, false);
    END LOOP;
END $$;
