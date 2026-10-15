import { randomUUID } from 'node:crypto';
import { join } from 'node:path';
import { DatabaseSync } from 'node:sqlite';

import { app } from 'electron';

import type { HistoryItem, HistoryKind } from '../shared/ipc';
import { broadcast } from './windows';

let database: DatabaseSync | null = null;

const db = () => {
    if (database) return database;

    database = new DatabaseSync(join(app.getPath('userData'), 'history.db'));

    database.exec(`
        CREATE TABLE IF NOT EXISTS history (
            id TEXT PRIMARY KEY,
            kind TEXT NOT NULL,
            name TEXT NOT NULL,
            localPath TEXT,
            url TEXT,
            shortUrl TEXT,
            fileId TEXT,
            size INTEGER NOT NULL DEFAULT 0,
            thumbnail TEXT,
            error TEXT,
            createdAt INTEGER NOT NULL
        );
        CREATE INDEX IF NOT EXISTS history_createdAt ON history (createdAt DESC);
    `);

    return database;
};

const toItem = (row: Record<string, unknown>) => row as unknown as HistoryItem;

export const addHistory = (item: {
    kind: HistoryKind;
    name: string;
    size: number;
    localPath?: string | null;
    thumbnail?: string | null;
}) => {
    const entry: HistoryItem = {
        id: randomUUID(),
        kind: item.kind,
        name: item.name,
        localPath: item.localPath ?? null,
        url: null,
        shortUrl: null,
        fileId: null,
        size: item.size,
        thumbnail: item.thumbnail ?? null,
        error: null,
        createdAt: Date.now(),
    };

    db()
        .prepare(
            `INSERT INTO history (id, kind, name, localPath, url, shortUrl, fileId, size, thumbnail, error, createdAt)
             VALUES (:id, :kind, :name, :localPath, :url, :shortUrl, :fileId, :size, :thumbnail, :error, :createdAt)`,
        )
        .run(entry);

    broadcast('history:changed', null);
    return entry;
};

export const updateHistory = (
    id: string,
    patch: Partial<Pick<HistoryItem, 'url' | 'shortUrl' | 'fileId' | 'error' | 'localPath'>>,
) => {
    const keys = Object.keys(patch) as (keyof typeof patch)[];
    if (!keys.length) return;

    db()
        .prepare(
            `UPDATE history SET ${keys.map((key) => `${key} = :${key}`).join(', ')} WHERE id = :id`,
        )
        .run({ ...patch, id } as never);

    broadcast('history:changed', null);
};

export const listHistory = (offset: number, limit: number) => {
    const items = db()
        .prepare('SELECT * FROM history ORDER BY createdAt DESC LIMIT :limit OFFSET :offset')
        .all({ limit, offset })
        .map(toItem);

    const { total } = db().prepare('SELECT COUNT(*) AS total FROM history').get() as {
        total: number;
    };

    return { items, total };
};

export const getHistory = (id: string) => {
    const row = db().prepare('SELECT * FROM history WHERE id = :id').get({ id });
    return row ? toItem(row) : null;
};

export const removeHistory = (id: string) => {
    db().prepare('DELETE FROM history WHERE id = :id').run({ id });
    broadcast('history:changed', null);
};

export const clearHistory = () => {
    db().exec('DELETE FROM history');
    broadcast('history:changed', null);
};
