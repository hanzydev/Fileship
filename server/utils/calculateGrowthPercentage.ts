export const calculateGrowthPercentage = (
    current: number | bigint,
    previous: number | bigint,
) => {
    if (current > Number.MAX_SAFE_INTEGER) current = Number.MAX_SAFE_INTEGER;
    if (previous > Number.MAX_SAFE_INTEGER) previous = Number.MAX_SAFE_INTEGER;

    current = Number(current);
    previous = Number(previous);

    if (previous === 0) return current === 0 ? 0 : 100;

    return Math.round(((current - previous) / previous) * 100);
};
