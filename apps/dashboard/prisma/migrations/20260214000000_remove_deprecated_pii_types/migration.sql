-- RemoveDeprecatedPiiTypes
UPDATE "File"
SET "piiReasons" = array_remove(
  array_remove(
    array_remove(
      array_remove(
        array_remove(
          array_remove(
            array_remove(
              array_remove(
                array_remove("piiReasons", 'EMAIL'),
              'PHONE'),
            'CREDIT_CARD'),
          'IP_ADDRESS'),
        'IBAN'),
      'GENERIC_ACCOUNT_ID'),
    'ADDRESS'),
  'VISUAL_MOBILE_PHONE'),
'VISUAL_PERSON')
WHERE "piiReasons" && ARRAY['EMAIL','PHONE','CREDIT_CARD','IP_ADDRESS','IBAN','GENERIC_ACCOUNT_ID','ADDRESS','VISUAL_MOBILE_PHONE','VISUAL_PERSON']::text[];

-- SetPiiDetectedFalseWhenEmpty
UPDATE "File"
SET "piiDetected" = false
WHERE "piiDetected" = true
  AND cardinality("piiReasons") = 0;