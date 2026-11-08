---
title: Storage
description: Keep Fileship's uploads and backups on the local disk, S3-compatible storage (AWS, Cloudflare R2, Backblaze B2, MinIO), SFTP or FTP, move between them and import existing files.
---

# Storage

Fileship keeps uploads and backups in its data directory by default (`data/` next to `docker-compose.yml`). In **Administration → Instance → Storage**, super admins can add S3-compatible storage, SFTP or FTP and choose which one new files go to.

## How storage works

- **New uploads and backups go to the active storage.** Each file stays where it was written, so switching storage does not move anything. **Move everything here** on a storage moves existing files and backups to it.
- **Uploads stream to the storage chunk by chunk.** Only images that need EXIF removal or compression are assembled in the data directory first.
- **Thumbnails, AI models and temporary files always stay in the data directory**, whatever storage is active. Keep enough local space for them.
- **Credentials are encrypted** with `data/secret.key`, so keep that file together with your data. The API never sends secrets back; when you edit a storage, leave a secret empty to keep it.
- **The connection is tested before saving** (**Test connection**).

On every start, Fileship removes files and backups whose data is missing from their storage. A storage that cannot be reached, cannot be listed or looks empty is skipped, so a misconfigured storage never deletes anything. Rows created in the last 15 minutes are skipped too.

## Add a storage

1. Open **Administration → Instance → Storage** and choose **Add storage**.
2. Give it a **Name** and choose the **Type**.
3. Fill in the fields below and choose **Test connection**, then **Save**.
4. In the storage's menu (**⋯**), choose **Set active** to send new uploads there.

## S3-compatible storage

| Field                 | What to enter                                                                                  |
| --------------------- | ---------------------------------------------------------------------------------------------- |
| **Endpoint**          | Leave empty for AWS. Needed for R2, MinIO and other S3 services, with `https://` or `http://`. |
| **Region**            | The bucket's region. Services without regions usually accept `auto` or `us-east-1`.            |
| **Bucket**            | The bucket name. Create it first; Fileship does not create buckets.                            |
| **Access key ID**     | The access key.                                                                                |
| **Secret access key** | The secret.                                                                                    |
| **Prefix**            | Optional folder inside the bucket, for example `fileship`.                                     |
| **Path-style URLs**   | Needed by MinIO and some self-hosted S3 services.                                              |

Fileship writes `uploads/<file name>` and `backups/<user id>/<id>.tgz` under the prefix. Uploads use S3 multipart uploads with one part per chunk, which is one reason the chunk size is at least 5 MB (S3's minimum part size).

The access key needs to list the bucket and read, write and delete objects, including multipart uploads. On AWS, this IAM policy is enough:

```json
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": ["s3:ListBucket"],
            "Resource": "arn:aws:s3:::my-fileship-bucket"
        },
        {
            "Effect": "Allow",
            "Action": [
                "s3:GetObject",
                "s3:PutObject",
                "s3:DeleteObject",
                "s3:AbortMultipartUpload"
            ],
            "Resource": "arn:aws:s3:::my-fileship-bucket/*"
        }
    ]
}
```

Files are served through Fileship, never straight from the bucket, so the bucket stays private. Reading a file counts as a download from the storage, which some providers bill for.

### AWS S3

| Field        | Value                                    |
| ------------ | ---------------------------------------- |
| **Endpoint** | empty                                    |
| **Region**   | the bucket's region, e.g. `eu-central-1` |

### Cloudflare R2

Create an API token with **Object Read & Write** for the bucket in **R2 → Manage API tokens**.

| Field                                     | Value                                           |
| ----------------------------------------- | ----------------------------------------------- |
| **Endpoint**                              | `https://<account id>.r2.cloudflarestorage.com` |
| **Region**                                | `auto`                                          |
| **Access key ID** / **Secret access key** | from the API token                              |

### Backblaze B2

Create an application key for the bucket. The endpoint is shown on the bucket's page.

| Field                 | Value                                                                                |
| --------------------- | ------------------------------------------------------------------------------------ |
| **Endpoint**          | `https://s3.<region>.backblazeb2.com`, e.g. `https://s3.us-west-004.backblazeb2.com` |
| **Region**            | the region part, e.g. `us-west-004`                                                  |
| **Access key ID**     | the key ID                                                                           |
| **Secret access key** | the application key                                                                  |

### MinIO

| Field               | Value                                     |
| ------------------- | ----------------------------------------- |
| **Endpoint**        | `http://minio:9000` or your MinIO address |
| **Region**          | `us-east-1`                               |
| **Path-style URLs** | on                                        |

Other S3 services (Wasabi, Hetzner Object Storage, SeaweedFS, Garage, …) work the same way with their endpoint and region.

> [!NOTE]
> S3 storage was tested with MinIO and SeaweedFS. The AWS, Cloudflare R2 and Backblaze B2 values follow those providers' documentation and were not tested against them.

## SFTP

| Field           | What to enter                                                 |
| --------------- | ------------------------------------------------------------- |
| **Host**        | The server's address.                                         |
| **Port**        | Usually 22.                                                   |
| **Username**    |                                                               |
| **Password**    | Use a password, a private key or both.                        |
| **Private key** | The private key in OpenSSH or PEM format.                     |
| **Folder**      | The folder Fileship writes into, for example `/srv/fileship`. |

## FTP

| Field        | What to enter                    |
| ------------ | -------------------------------- |
| **Host**     | The server's address.            |
| **Port**     | Usually 21.                      |
| **Username** |                                  |
| **Password** |                                  |
| **Folder**   | The folder Fileship writes into. |
| **Use TLS**  | Connect with explicit FTPS.      |

FTP can only append to a file, so uploads to an FTP storage send one chunk at a time instead of four. Prefer SFTP or S3 when you can.

## Move everything to another storage

**Move everything here** in a storage's menu copies every file and backup from the other storages to it, then removes them where they were. It runs in the background and shows its progress on the storage card. An import and a move never run at the same time. Make the target storage active first, so new uploads go there too.

## Delete a storage

**Delete** in a storage's menu makes Fileship forget it. Nothing is deleted from the storage itself. A storage that still holds files or backups, or is active, cannot be deleted (**Storage is in use**); move them away first.

## Import files that are already on a storage

**Import files** in a storage's menu brings files Fileship has no record of into a user's account, for example a folder of old screenshots:

- On the **Data directory**, put the files in the import folder (`data/import`) on the server. The dialog shows its real path.
- On S3, SFTP or FTP, every file on the storage that Fileship has no record of is imported, except Fileship's own `backups` folder.

Files are moved into `uploads/` on the same storage, so their old paths stop working. Each folder in a path becomes a Fileship folder inside the one above it, up to 10 levels deep. Files the user already has with the same name and size are skipped, so running it again is safe. Files changed in the last 15 minutes are left for the next run, and the user's storage limit applies.

To move a whole Zipline, XBackBone or Chibisafe instance, use **Import from another app** instead; see [Moving from another app](../migrating.md).

## Duplicates and storage reads

To find duplicates (**Settings → Storage → Duplicates**), Fileship reads a file back once and stores its SHA-256 hash, but only when another file of the same user has exactly the same size. On S3, SFTP and FTP this read counts as a download from the storage. See [Duplicates](../features/duplicates.md).

## Troubleshooting

**Could not connect** and **Could not list the storage** errors, and storages that look empty, are covered in [Storage problems](../troubleshooting/storage.md).
