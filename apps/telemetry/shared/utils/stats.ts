export const STAT_RANGES = ['30d', '90d', '1y', 'all'] as const;

export const STAT_RANGE_DAYS: Record<(typeof STAT_RANGES)[number], number | null> = {
    '30d': 30,
    '90d': 90,
    '1y': 365,
    all: null,
};

export const RANGE_LABELS: Record<(typeof STAT_RANGES)[number], string> = {
    '30d': '30 days',
    '90d': '90 days',
    '1y': '1 year',
    all: 'All',
};

export const ACTIVE_DAYS = 7;
