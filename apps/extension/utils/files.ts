const DATABASE = 'fileship';
const STORE = 'files';

const openDatabase = () =>
    new Promise<IDBDatabase>((resolve, reject) => {
        const request = indexedDB.open(DATABASE, 1);

        request.onupgradeneeded = () => request.result.createObjectStore(STORE);
        request.onsuccess = () => resolve(request.result);
        request.onerror = () => reject(request.error);
    });

const transact = async <T>(
    mode: IDBTransactionMode,
    run: (store: IDBObjectStore) => IDBRequest<T>,
) => {
    const database = await openDatabase();

    return new Promise<T>((resolve, reject) => {
        const request = run(database.transaction(STORE, mode).objectStore(STORE));

        request.onsuccess = () => resolve(request.result);
        request.onerror = () => reject(request.error);
    }).finally(() => database.close());
};

export const stashFiles = (files: File[]) =>
    Promise.all(
        files.map(async (file) => {
            const id = crypto.randomUUID();
            await transact('readwrite', (store) => store.put(file, id));
            return { id, name: file.name };
        }),
    );

export const takeFile = async (id: string) => {
    const file = await transact<File | undefined>('readonly', (store) => store.get(id));
    await transact('readwrite', (store) => store.delete(id));
    return file;
};

export const clearFiles = () => transact('readwrite', (store) => store.clear());
