---
title: Storage problems
description: Fix "Could not connect" and "Could not list the storage" errors for S3, SFTP and FTP storage in Fileship, storages that look empty and storages that can't be deleted.
---

# Storage problems

## Could not connect: …

**Test connection** or saving a storage failed. The text after the colon comes from the storage:

**S3**

| The error contains                                  | Fix                                                                                                            |
| --------------------------------------------------- | -------------------------------------------------------------------------------------------------------------- |
| `NoSuchBucket`, `404`                               | The bucket name or region is wrong, or the bucket does not exist. Create it first.                             |
| `InvalidAccessKeyId`, `SignatureDoesNotMatch`       | Wrong access key or secret.                                                                                    |
| `AccessDenied`, `403`                               | The key may not list or write the bucket. See the [policy](../configuration/storage.md#s3-compatible-storage). |
| `PermanentRedirect`, `AuthorizationHeaderMalformed` | The **Region** does not match the bucket's region.                                                             |
| `getaddrinfo ENOTFOUND <bucket>.<host>`             | The service needs **Path-style URLs** (MinIO and most self-hosted services).                                   |
| `ECONNREFUSED`, timeouts                            | The **Endpoint** is wrong or not reachable from the Fileship server.                                           |

**SFTP**

| The error contains                             | Fix                                                                                   |
| ---------------------------------------------- | ------------------------------------------------------------------------------------- |
| `All configured authentication methods failed` | Wrong username, password or private key.                                              |
| `Cannot parse privateKey`                      | Paste the whole key, including the `BEGIN` and `END` lines, in OpenSSH or PEM format. |
| `No such file`                                 | The **Folder** does not exist. Create it first.                                       |
| `Permission denied`                            | The user may not write the folder.                                                    |

**FTP**

| The error contains        | Fix                                                                                     |
| ------------------------- | --------------------------------------------------------------------------------------- |
| `530`                     | Wrong username or password.                                                             |
| `550`                     | The **Folder** does not exist or is not writable.                                       |
| timeouts after connecting | Passive mode ports are blocked by a firewall, or **Use TLS** does not match the server. |

## Could not list the storage: …

The storage could be reached, but listing its files failed, during an import or at startup. Check that the key may list the bucket (`s3:ListBucket`) or read the folder.

## The storage looks empty

At startup, Fileship removes files whose bytes are missing from their storage. When a storage cannot be listed or returns nothing while Fileship has files on it, Fileship skips it and logs a warning instead of deleting anything. Fix the storage settings (wrong bucket, prefix or folder), then restart. Nothing was deleted.

## Storage is in use

A storage cannot be deleted while it is active or still holds files or backups. Make another storage active, then use **Move everything here** on it, and delete the old one afterwards.

## A migration is already running / Wait for the import to finish

Only one storage move or import runs at a time. Wait until the progress on the storage card finishes.

## Files on S3 open slowly

Every view reads the file from the storage through Fileship. Choose a region near your server, or use the data directory for files you open often. Thumbnails always come from the data directory.
