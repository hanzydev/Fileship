---
title: Duplicates
description: How Fileship finds files with the same content, warns before you upload one again, and helps you free the space.
---

# Duplicates

Fileship recognises files with exactly the same content, even under different names.

## Before an upload

When you upload from the dashboard, Fileship checks whether you already have a file of the same size. If so, your browser computes the file's hash and compares it. A match asks what to do: **Upload anyway** or **Skip duplicates**.

The check needs a secure context (HTTPS or `http://localhost`) and only covers files up to 256 MB. Images that Fileship re-encodes (EXIF removal or compression) and files the browser could not check are uploaded, and you get a notice afterwards if the stored file turns out to be a duplicate (**You already had this file**).

## Finding duplicates

**Settings → Storage → Duplicates** lists groups of files with the same content, the ones that waste the most space first, and how much space **can be freed**. **Keep oldest** moves the other copies to the trash; or open each file and decide.

## How it works

Fileship stores a SHA-256 hash only for files that have the same size as another file of the same user, so most files are never read again. The hash is made in the background after an upload, at startup and after a restore from the trash. Files in the trash are not compared. On S3, SFTP and FTP storage, hashing reads the file once, which counts as a download from the storage.
