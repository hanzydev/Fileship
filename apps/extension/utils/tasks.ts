export type TaskKind = 'file' | 'image' | 'screenshot' | 'link';

export type Task = {
    id: string;
    kind: TaskKind;
    name: string;
    status: 'running' | 'done' | 'failed';
    progress: number;
    url?: string;
    error?: string;
};

export const tasksItem = storage.defineItem<Task[]>('session:tasks', { fallback: [] });
