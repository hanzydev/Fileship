export const predictExpiration = (date: Date | string | null) => {
    if (!date) return 'Never';

    const diff = new Date(date).getTime() - Date.now();

    return EXPIRATIONS.slice(1).reduce((prev, curr) =>
        Math.abs(curr.value! - diff) < Math.abs(prev.value! - diff) ? curr : prev,
    ).label;
};
