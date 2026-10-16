# AI features

Fileship can analyze uploads to power smart search, image captions, OCR and sensitive content detection. The models run locally with [Transformers.js](https://huggingface.co/docs/transformers.js); no file ever leaves your server.

AI features are **enabled by default**. Super admins can turn them off in **Administration → Instance**, and each user can opt out in their own settings. The `NUXT_PUBLIC_AI_ENABLED` environment variable sets the instance default.

When AI is disabled:

- no new AI jobs are queued or processed
- smart search falls back to full-text search
- AI-related parts of the interface are hidden

Every image, video and text file is analyzed once in the background after it is uploaded. **Parallel jobs** sets how many files are analyzed at the same time and **Threads per job** how many CPU cores each one uses; on small servers keep both low. A job whose server stops responding is picked up again after **Job heartbeat timeout**, and a file that fails three times waits until you choose **Retry failed** in the AI section of **Administration → Instance**, which also shows the queue. Failed jobs are cleared after 30 days, and those files are then tried again. **Reset AI data** there deletes the captions, OCR text, search data and sensitive content flags of every file and analyzes everything again; choices made with **Not sensitive** or **Mark as sensitive** are kept.

## AI presets

The preset sets which models are used and how long they stay in memory. Choose it in **Administration → Instance** or with `NUXT_AI_PRESET`. A new preset applies to files analyzed from then on. Files that were already analyzed keep their captions and text until you choose **Reset AI data**. The hardware each preset needs is listed in [Installation](installation.md#requirements).

|                        | `low`                          | `medium` (default)                    | `high`                                 |
| ---------------------- | ------------------------------ | ------------------------------------- | -------------------------------------- |
| Image and video search | mSigLIP, compact query encoder | mSigLIP, full precision query encoder | mSigLIP, full precision query encoder  |
| Captions               | short                          | detailed                              | detailed, full precision image encoder |
| OCR                    | PaddleOCR tiny                 | PaddleOCR small                       | PaddleOCR small                        |
| Video frames           | 8                              | 16                                    | 32                                     |
| Models stay loaded for | 2 minutes                      | 5 minutes                             | 15 minutes                             |

## Models

The models are downloaded from Hugging Face the first time they are needed and kept in `data/models`, so updates and both blue/green containers reuse them. To run without internet access, copy that folder from another installation.

| Task                                   | Model                                                                                                                                                                  |
| -------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Image and video search                 | [mSigLIP base](https://huggingface.co/google/siglip-base-patch16-256-multilingual) ([ONNX](https://huggingface.co/pulsejet/siglip-base-patch16-256-multilingual-onnx)) |
| Captions                               | [Florence-2 base](https://huggingface.co/onnx-community/Florence-2-base)                                                                                               |
| OCR                                    | [PaddleOCR PP-OCRv6 and PP-OCRv5 script models](https://github.com/PT-Perkasa-Pilar-Utama/ppu-paddle-ocr)                                                              |
| Search in captions, OCR and text files | [Multilingual MiniLM](https://huggingface.co/Xenova/paraphrase-multilingual-MiniLM-L12-v2)                                                                             |
| Sensitive content                      | [OpenAI Privacy Filter](https://huggingface.co/openai/privacy-filter)                                                                                                  |

The models were chosen by measuring them on 400 images of [Crossmodal-3600](https://google.github.io/crossmodal-3600/) with human captions in English, Turkish, German, Spanish, French, Arabic, Russian, Japanese and Chinese. Compared with v5, the `medium` preset finds the right image first for 76% of non-English queries instead of 58%, and a query that describes nothing in the library returns about 1.5 results instead of 12. OCR reads Latin, Cyrillic, Greek, Thai, Korean, Japanese and Chinese text (Arabic only partly), and no longer invents text on photos that have none.

## Smart search

Smart search combines what images and videos show, what their captions and OCR text say, the content of text files, and file names. Queries work in many languages, so `köpek` finds the same photos as `dog`. Only results that clearly match are shown; a query that matches nothing returns few or no results. While a file is still being analyzed, full-text search already finds it by name.

## Sensitive content

Fileship reads the text of documents and the OCR text of images with the [OpenAI Privacy Filter](https://huggingface.co/openai/privacy-filter) model, which looks at the context instead of fixed patterns, so it works with any country's formats. A file is marked as sensitive when it contains:

- private email addresses, phone numbers or home addresses
- account, ID, tax or social security numbers
- passwords and secret keys
- payment card numbers (checked with the Luhn algorithm), IBANs (checked with their checksum) and the machine-readable lines of passports and ID cards
- photos of payment cards and ID documents that show a number

Public information such as company addresses, support emails, brands and public figures is not sensitive. Faces are not sensitive, and a name or a personal date only counts next to something else, or when they appear together.

Sensitive files are hidden from public folders and their archives, and can be blurred for you with **Hide sensitive previews** in your AI settings. While AI is enabled, images and text files are also kept out of public folders until they have been checked.

If Fileship gets it wrong, choose **Not sensitive** or **Mark as sensitive** from the file's menu. Your choice is kept when the file is checked again.
