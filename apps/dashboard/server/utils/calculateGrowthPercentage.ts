export const calculateGrowthPercentage = (current: number | bigint, previous: number | bigint) => {
    current = Number(current);
    previous = Number(previous);

    if (previous === 0) return current === 0 ? 0 : 100;

    return Math.round(((current - previous) / previous) * 100);
};
