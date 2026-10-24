export const useTasks = () => {
    const tasks = ref<Task[]>([]);

    tasksItem.getValue().then((value) => (tasks.value = value));
    onScopeDispose(tasksItem.watch((value) => (tasks.value = value ?? [])));

    return tasks;
};
