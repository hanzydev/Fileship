-- AlterTable
ALTER TABLE "File" ADD COLUMN IF NOT EXISTS "piiOverride" BOOLEAN;

-- Remove the "null" strings left by empty OCR and caption results
UPDATE "File" SET "ocrText" = '' WHERE "ocrText" = 'null';
UPDATE "File" SET "caption" = '' WHERE "caption" = 'null';

-- Faces, signatures, organizations, places and unvalidated patterns are no longer sensitive
UPDATE "File"
SET "piiReasons" = ARRAY(
    SELECT DISTINCT CASE
        WHEN "reason" IN ('VISUAL_CREDIT_CARD', 'VISUAL_DEBIT_CARD', 'CREDIT_CARD') THEN 'PAYMENT_CARD'
        WHEN "reason" IN ('VISUAL_IDENTITY_CARD', 'VISUAL_PASSPORT') THEN 'ID_DOCUMENT'
        WHEN "reason" = 'NER_PER' THEN 'PERSON_NAME'
        ELSE "reason"
    END
    FROM unnest("piiReasons") AS "reason"
    WHERE "reason" IN (
        'VISUAL_CREDIT_CARD',
        'VISUAL_DEBIT_CARD',
        'VISUAL_IDENTITY_CARD',
        'VISUAL_PASSPORT',
        'CREDIT_CARD',
        'NER_PER',
        'EMAIL',
        'PHONE',
        'PAYMENT_CARD',
        'NATIONAL_ID',
        'PASSPORT',
        'ID_DOCUMENT',
        'PERSON_NAME'
    )
)
WHERE cardinality("piiReasons") > 0;

-- A name alone is not sensitive
UPDATE "File" SET "piiReasons" = '{}' WHERE "piiReasons" = ARRAY['PERSON_NAME'];

UPDATE "File" SET "piiDetected" = cardinality("piiReasons") > 0;

-- Re-check every file with the new rules
DELETE FROM "AIJob" WHERE "type"::text = 'DetectPII';
